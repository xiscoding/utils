"""
wrapper around run.c
mostly deals with the sentencepiece encoding/decoding
C code does all the transformer inference of the individual tokens
"""
from tokenizer import Tokenizer
import subprocess
import time

# specify your command
'''
run_with_text.c: Usage: %s <checkpoint_file> <input_sequence> [temperature] [seed]\n"
'''
command = ["./run", "out/model.bin", "Once upon a time there was a bird, "]

# Start the process
proc = subprocess.Popen(command, stdout=subprocess.PIPE)
enc = Tokenizer()

t0 = time.time()
tokens = []
last = ''
for line in proc.stdout:
    try:
        token = int(line.decode('utf-8').strip())
        dec = enc.decode(tokens + [token])
        chunk = dec[len(last):]
        print(chunk, end='',flush=True)
        tokens.append(token)
        last = dec
    except ValueError as e:
        print(f"Error occurred: {e}")
        break  # Break the loop if error occurs

t1 = time.time()

print(f"\nachieved tok/s: {len(tokens) / (t1 - t0)}")
proc.wait()


# from tokenizer import Tokenizer
# import subprocess
# import time

# # specify your command
# command = ["./run", "/home/xdoestech/llama2.c/data/alpaca_cota/Auto-CoT.bin"]

# # Start the process
# proc = subprocess.Popen(command, stdout=subprocess.PIPE)
# enc = Tokenizer()

# t0 = time.time()
# tokens = []
# last = ''
# for line in proc.stdout:
#     token = int(line.decode('utf-8').strip())
#     dec = enc.decode(tokens + [token])
#     chunk = dec[len(last):]
#     print(chunk, end='',flush=True)
#     tokens.append(token)
#     last = dec
# t1 = time.time()
# # seeking help: how can we do streaming inference in sentencepiece properly?
# # or even delete sentencepiece entirely?

# print(f"\nachieved tok/s: {len(tokens) / (t1 - t0)}")
# proc.wait()
