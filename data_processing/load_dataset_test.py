''' VICUNA DATASET INFO
DatasetDict({
    train: Dataset({
        features: ['id', 'conversations'],
        num_rows: 34598
    })
})
'''

''' ORCA DATASET INFO:

DatasetDict({
    train: Dataset({
        features: ['id', 'system_prompt', 'question', 'response'],
        num_rows: 4233923
    })
})

'''

from datasets import load_dataset

dataset_name = "ehartford/wizard_vicuna_70k_unfiltered"
# open-orca is not split
# load_dataset 'man': https://huggingface.co/docs/datasets/v2.14.3/en/package_reference/loading_methods#datasets.load_dataset

dataset = load_dataset(dataset_name)

print(dataset)

