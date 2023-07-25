/*
Inference for Llama-2 Transformer model in pure C.

Compile simply with:
$ gcc -o run run.c
Or if that doesn't work then:
$ gcc -o run run.c -lm

Then run with:
$ ./run
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// ----------------------------------------------------------------------------
// Transformer and RunState structs, and related memory management

typedef struct {
    int dim; // transformer dimension
    int hidden_dim; // for ffn layers
    int n_layers; // number of layers
    int n_heads; // number of query heads
    int n_kv_heads; // number of key/value heads (can be < query heads because of multiquery)
    int vocab_size; // vocabulary size, usually 256 (byte-level)
    int seq_len; // max sequence length
} Config;

typedef struct {

    
    // token embedding table
    float* token_embedding_table;    // (vocab_size, dim)
    //size of token embedding table 
    int size_of_token_embedding_table;
    // weights for rmsnorms
    float* rms_att_weight; // (layer, dim) rmsnorm weights
    float* rms_ffn_weight; // (layer, dim)
    // weights for matmuls
    float* wq; // (layer, dim, dim)
    float* wk; // (layer, dim, dim)
    float* wv; // (layer, dim, dim)
    float* wo; // (layer, dim, dim)
    // weights for ffn
    float* w1; // (layer, hidden_dim, dim)
    float* w2; // (layer, dim, hidden_dim)
    float* w3; // (layer, hidden_dim, dim)
    // final rmsnorm
    float* rms_final_weight; // (dim,)
    // freq_cis for RoPE relatively positional embeddings
    float* freq_cis_real; // (seq_len, dim/2)
    float* freq_cis_imag; // (seq_len, dim/2)
} TransformerWeights;

typedef struct {
    // current wave of activations
    float *x; // activation at current time stamp (dim,)
    float *xb; // same, but inside a residual branch (dim,)
    float *xb2; // an additional buffer just for convenience (dim,)
    float *hb; // buffer for hidden dimension in the ffn (hidden_dim,)
    float *hb2; // buffer for hidden dimension in the ffn (hidden_dim,)
    float *q; // query (dim,)
    float *k; // key (dim,)
    float *v; // value (dim,)
    float *att; // buffer for scores/attention values (seq_len,)
    float *logits; // output logits
    // kv cache
    float* key_cache;   // (layer, seq_len, dim)
    //size of key chace
    int size_of_key_cache;
    float* value_cache; // (layer, seq_len, dim)
} RunState;

void malloc_run_state(RunState* s, Config* p) {
    // we calloc instead of malloc to keep valgrind happy
    s->x = calloc(p->dim, sizeof(float));
    s->xb = calloc(p->dim, sizeof(float));
    s->xb2 = calloc(p->dim, sizeof(float));
    s->hb = calloc(p->hidden_dim, sizeof(float));
    s->hb2 = calloc(p->hidden_dim, sizeof(float));
    s->q = calloc(p->dim, sizeof(float));
    s->k = calloc(p->dim, sizeof(float));
    s->v = calloc(p->dim, sizeof(float));
    s->att = calloc(p->seq_len, sizeof(float));
    s->logits = calloc(p->vocab_size, sizeof(float));
    s->key_cache = calloc(p->n_layers * p->seq_len * p->dim, sizeof(float));
    s->size_of_key_cache = p->dim * p->seq_len * p->n_layers;
    s->value_cache = calloc(p->n_layers * p->seq_len * p->dim, sizeof(float));
    // ensure all mallocs went fine
    if (!s->x || !s->xb || !s->xb2 || !s->hb || !s->hb2 || !s->q 
     || !s->k || !s->v || !s->att || !s->logits || !s->key_cache 
     || !s->value_cache) {
        printf("malloc failed!\n");
        exit(1);
    }
}

void free_run_state(RunState* s, Config* p) {
    free(s->x);
    free(s->xb);
    free(s->xb2);
    free(s->hb);
    free(s->hb2);
    free(s->q);
    free(s->k);
    free(s->v);
    free(s->att);
    free(s->logits);
    free(s->key_cache);
    free(s->value_cache);
}

void malloc_weights(TransformerWeights* w, Config* p) {
    // we calloc instead of malloc to keep valgrind happy
    
    w->token_embedding_table = calloc(p->vocab_size * p->dim, sizeof(float));
   //gpt addition to check for size
    w->size_of_token_embedding_table = p->dim * p->vocab_size;
    w->rms_att_weight = calloc(p->n_layers * p->dim, sizeof(float));
    w->rms_ffn_weight = calloc(p->n_layers * p->dim, sizeof(float));
    w->wq = calloc(p->n_layers * p->dim * p->dim, sizeof(float));
    w->wk = calloc(p->n_layers * p->dim * p->dim, sizeof(float));
    w->wv = calloc(p->n_layers * p->dim * p->dim, sizeof(float));
    w->wo = calloc(p->n_layers * p->dim * p->dim, sizeof(float));
    w->w1 = calloc(p->n_layers * p->hidden_dim * p->dim, sizeof(float));
    w->w2 = calloc(p->n_layers * p->dim * p->hidden_dim, sizeof(float));
    w->w3 = calloc(p->n_layers * p->hidden_dim * p->dim, sizeof(float));
    w->rms_final_weight = calloc(p->dim, sizeof(float));
    w->freq_cis_real = calloc(p->seq_len * p->dim / 2, sizeof(float));
    w->freq_cis_imag = calloc(p->seq_len * p->dim / 2, sizeof(float));
    // ensure all mallocs went fine
    if (!w->token_embedding_table || !w->rms_att_weight || !w->rms_ffn_weight 
     || !w->wq || !w->wk || !w->wv || !w->wo || !w->w1 || !w->w2 || !w->w3 || 
        !w->rms_final_weight || !w->freq_cis_real || !w->freq_cis_imag) {
        printf("malloc failed!\n");
        exit(1);
    }
}

void free_weights(TransformerWeights* w, Config* p) {
    free(w->token_embedding_table);
    free(w->rms_att_weight);
    free(w->rms_ffn_weight);
    free(w->wq);
    free(w->wk);
    free(w->wv);
    free(w->wo);
    free(w->w1);
    free(w->w2);
    free(w->w3);
    free(w->rms_final_weight);
    free(w->freq_cis_real);
    free(w->freq_cis_imag);
}

// ----------------------------------------------------------------------------
// initialization: random init, or read from checkpoint

void checkpoint_init_weights(TransformerWeights *w, Config* p, FILE* f) {
    fread(w->token_embedding_table, sizeof(float), p->vocab_size * p->dim, f);
    fread(w->rms_att_weight, sizeof(float), p->n_layers * p->dim, f);
    fread(w->wq, sizeof(float), p->n_layers * p->dim * p->dim, f);
    fread(w->wk, sizeof(float), p->n_layers * p->dim * p->dim, f);
    fread(w->wv, sizeof(float), p->n_layers * p->dim * p->dim, f);
    fread(w->wo, sizeof(float), p->n_layers * p->dim * p->dim, f);
    fread(w->rms_ffn_weight, sizeof(float), p->n_layers * p->dim, f);
    fread(w->w1, sizeof(float), p->n_layers * p->dim * p->hidden_dim, f);
    fread(w->w2, sizeof(float), p->n_layers * p->hidden_dim * p->dim, f);
    fread(w->w3, sizeof(float), p->n_layers * p->dim * p->hidden_dim, f);
    fread(w->rms_final_weight, sizeof(float), p->dim, f);
    int head_size = p->dim / p->n_heads;
    fread(w->freq_cis_real, sizeof(float), p->seq_len * head_size / 2, f);
    fread(w->freq_cis_imag, sizeof(float), p->seq_len * head_size / 2, f);
}

// ----------------------------------------------------------------------------
// neural net blocks

void copy(float *a, float *b, int size) {
    for (int i = 0; i < size; i++) {
        a[i] = b[i];
    }
}

void accum(float *a, float *b, int size) {
    for (int i = 0; i < size; i++) {
        a[i] += b[i];
    }
}

void rmsnorm(float* o, float* x, float* weight, int size) {
    // calculate sum of squares
    float ss = 0.0f;
    for (int j = 0; j < size; j++) {
        ss += x[j] * x[j];
    }
    ss /= size;
    ss += 1e-5f;
    ss = 1.0f / sqrt(ss);
    // normalize and scale
    for (int j = 0; j < size; j++) {
        o[j] = weight[j] * (ss * x[j]);
    }
}

void softmax(float* x, int size) {
    if(size == 1) {
        x[0] = 1.0f;
        return;
    }
    // find max value (for numerical stability)
    float max_val = x[0];
    for (int i = 1; i < size; i++) {
        if (x[i] > max_val) {
            max_val = x[i];
        }
    }
    // e^x
    for (int i = 0; i < size; i++) {
        x[i] = exp(x[i] - max_val);
    }
    // normalize
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        sum += x[i];
    }
    for (int i = 0; i < size; i++) {
        x[i] /= sum;
    }
}

void matmul(float* xout, float* x, float* w, int n, int d) {
    // W (d,n) @ x (n,) -> xout (d,)
    for (int i = 0; i < d; i++) {
        float val = 0.0f;
        for (int j = 0; j < n; j++) {
            val += w[i * n + j] * x[j];
        }
        xout[i] = val;
    }
}

void transformer(int token, int pos, Config* p, RunState* s, TransformerWeights* w) {
    
    // a few convenice variables
    float *x = s->x;
    int dim = p->dim;
    int hidden_dim =  p->hidden_dim;
    int head_size = dim / p->n_heads;


// Check if token is within the valid range
    if (token < 0 || token >= (w->size_of_token_embedding_table / dim)) {
        fprintf(stderr, "Error: Invalid token passed to transformer function.\n");
        return;
    }

    // copy the token embedding into x
    float* content_row = &(w->token_embedding_table[token * dim]);
    copy(x, content_row, dim);

    // Check if pos is within the valid range
    if (pos < 0 || pos >= (s->size_of_key_cache / dim)) {
        fprintf(stderr, "Error: Invalid pos passed to transformer function.\n");
        return;
    }
    // // copy the token embedding into x
    // float* content_row = &(w->token_embedding_table[token * dim]);
    // copy(x, content_row, dim);

    // pluck out the "pos" row of freq_cis_real and freq_cis_imag
    float* freq_cis_real_row = w->freq_cis_real + pos * head_size / 2;
    float* freq_cis_imag_row = w->freq_cis_imag + pos * head_size / 2;

    // forward all the layers
    for(int l = 0; l < p->n_layers; l++) {
    
        // attention rmsnorm
        rmsnorm(s->xb, x, w->rms_att_weight + l*dim, dim);

        // qkv matmuls for this position
        matmul(s->q, s->xb, w->wq + l*dim*dim, dim, dim);
        matmul(s->k, s->xb, w->wk + l*dim*dim, dim, dim);
        matmul(s->v, s->xb, w->wv + l*dim*dim, dim, dim);

        // apply RoPE rotation to the q and k vectors for each head
        for (int h = 0; h < p->n_heads; h++) {
            // get the q and k vectors for this head
            float* q = s->q + h * head_size;
            float* k = s->k + h * head_size;
            // rotate q and k by the freq_cis_real and freq_cis_imag
            for (int i = 0; i < head_size; i+=2) {
                float q0 = q[i];
                float q1 = q[i+1];
                float k0 = k[i];
                float k1 = k[i+1];
                float fcr = freq_cis_real_row[i/2];
                float fci = freq_cis_imag_row[i/2];
                q[i]   = q0 * fcr - q1 * fci;
                q[i+1] = q0 * fci + q1 * fcr;
                k[i]   = k0 * fcr - k1 * fci;
                k[i+1] = k0 * fci + k1 * fcr;
            }
        }

        // save key,value at this time step (pos) to our kv cache
        int loff = l * p->seq_len * dim; // kv cache layer offset for convenience
        float* key_cache_row = s->key_cache + loff + pos * dim;
        float* value_cache_row = s->value_cache + loff + pos * dim;
        copy(key_cache_row, s->k, dim);
        copy(value_cache_row, s->v, dim);
        
        // multihead attention. iterate over all heads
        for (int h = 0; h < p->n_heads; h++) {
            // get the query vector for this head
            float* q = s->q + h * head_size;
            // iterate over all timesteps, including the current one
            for (int t = 0; t <= pos; t++) {
                // get the key vector for this head and at this timestep
                float* k = s->key_cache + loff + t * dim + h * head_size;
                // calculate the attention score as the dot product of q and k
                float score = 0.0f;
                for (int i = 0; i < head_size; i++) {
                    score += q[i] * k[i];
                }
                score /= sqrtf(head_size);
                // save the score to the attention buffer
                s->att[t] = score;
            }

            // softmax the scores to get attention weights, from 0..pos inclusively
            softmax(s->att, pos + 1);
            
            // weighted sum of the values, store back into xb
            for (int i = 0; i < head_size; i++) {
                float val = 0.0f;
                for (int t = 0; t <= pos; t++) {
                    val += s->att[t] * s->value_cache[loff + t * dim + h * head_size + i]; // note bad locality
                }
                s->xb[h * head_size + i] = val;
            }
        }

        // final matmul to get the output of the attention
        matmul(s->xb2, s->xb, w->wo + l*dim*dim, dim, dim);

        // residual connection back into x
        accum(x, s->xb2, dim);

        // ffn rmsnorm
        rmsnorm(s->xb, x, w->rms_ffn_weight + l*dim, dim);

        // Now for FFN in PyTorch we have: self.w2(F.silu(self.w1(x)) * self.w3(x))
        // first calculate self.w1(x) and self.w3(x)
        matmul(s->hb, s->xb, w->w1 + l*dim*hidden_dim, dim, hidden_dim);
        matmul(s->hb2, s->xb, w->w3 + l*dim*hidden_dim, dim, hidden_dim);
        
        // F.silu; silu(x)=x*σ(x),where σ(x) is the logistic sigmoid
        for (int i = 0; i < hidden_dim; i++) {
            s->hb[i] = s->hb[i] * (1.0f / (1.0f + expf(-s->hb[i])));
        }
        
        // elementwise multiply with w3(x)
        for (int i = 0; i < hidden_dim; i++) {
            s->hb[i] = s->hb[i] * s->hb2[i];
        }

        // final matmul to get the output of the ffn
        matmul(s->xb, s->hb, w->w2 + l*dim*hidden_dim, hidden_dim, dim);

        // residual connection
        accum(x, s->xb, dim);
    }
    
    // final rmsnorm
    rmsnorm(x, x, w->rms_final_weight, dim);

    // classifier into logits
    matmul(s->logits, x, w->token_embedding_table, p->dim, p->vocab_size);
}

int sample(float* probabilities, int n) {
    // sample index from probabilities, they must sum to 1
    float r = (float)rand() / (float)RAND_MAX;
    float cdf = 0.0f;
    for (int i = 0; i < n; i++) {
        cdf += probabilities[i];
        if (r < cdf) {
            return i;
        }
    }
    return n - 1; // in case of rounding errors
}

int argmax(float* v, int n) {
    // return argmax of v in elements 0..n
    int max_i = 0;
    float max_p = v[0];
    for (int i = 1; i < n; i++) {
        if (v[i] > max_p) {
            max_i = i;
            max_p = v[i];
        }
    }
    return max_i;
}

// ----------------------------------------------------------------------------

//NOTE:gpt addition main function modified to accpet text input
//gcc -O3 -o run run_with_text.c -lm
//./run out/model.bin

#define MAX_INPUT_LENGTH 2048

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    setbuf(stdout, NULL); // disable stdout buffering

    // poor man's C argparse
    char *checkpoint = NULL;
    float temperature = 0.9f;
    // 'checkpoint' and 'input_sequence' are necessary args
    if (argc < 3) {
        printf("Usage: %s <checkpoint_file> <input_sequence> [temperature] [seed]\n", argv[0]);
        return 1;
    }
    checkpoint = argv[1];
    char *input_sequence_str = argv[2];
    // temperature is optional
    if (argc >= 4) {
        temperature = atof(argv[3]);
    }
    // seed is optional
    if (argc >= 5) {
        unsigned int seed = atoi(argv[4]);
        srand(seed);
    } else {
        time_t current_time; 
        time(&current_time);
        srand((unsigned int)current_time);
    }

    // call Python script to tokenize input sequence
    char command[2048];  // make sure this is large enough
    sprintf(command, "python pretokenize.py \"%s\"", input_sequence_str);
    FILE *pipe = popen(command, "r");
    if (!pipe) {
        printf("Unable to open pipe!");
        return 1;
    }

    // read tokenized input sequence from pipe
    int input_sequence[MAX_INPUT_LENGTH];
    int input_length = 0;
    while (fscanf(pipe, "%d", &input_sequence[input_length]) == 1) {
        input_length++;
        
    }
    //printf("input_length: %d", input_length);
    pclose(pipe);

    // read in the config header
    Config config;
    FILE *file = fopen(checkpoint, "rb");
    if (!file) {
        printf("Unable to open file!");
        return 1;
    }
    fread(&config, sizeof(Config), 1, file);
    //printf("config.seq_len: %d", config.seq_len);
    // check if the fread was successful
    if (ferror(file)) {
        printf("Error reading file!");
        fclose(file);
        return 1;
    }

    // create and init the Transformer
    TransformerWeights weights;
    malloc_weights(&weights, &config);
    checkpoint_init_weights(&weights, &config, file);
    fclose(file);

    // create and init the application RunState
    RunState state;
    malloc_run_state(&state, &config);

    // initialize the model's state with the input sequence
    int next;
    int pos = 0;

    
    for (int i = 0; i < input_length; i++) {
        transformer(input_sequence[i], pos, &config, &state, &weights);
        pos++;
        next = input_sequence[i];  // add this line

    }

    // start generating the output sequence after the input sequence
    while (pos < config.seq_len) {

        // forward the transformer to get logits for the next token
        transformer(next, pos, &config, &state, &weights);

        // sample the next token
        if(temperature == 0.0f) {
            // greedy argmax sampling
            next = argmax(state.logits, config.vocab_size);
        } else {
            // apply the temperature to the logits
            for (int q=0; q<config.vocab_size; q++) { state.logits[q] /= temperature; }
            // apply softmax to the logits to get the probabilities for next token
            softmax(state.logits, config.vocab_size);
            // we now want to sample from this distribution to get the next token
            next = sample(state.logits, config.vocab_size);
        }
        printf("%d\n", next);
        // advance forward
        pos++;
    }
    free_run_state(&state, &config);
    free_weights(&weights, &config);
    return 0;
}