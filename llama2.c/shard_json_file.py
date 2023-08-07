'''
Shards json files
'''

import os
import json


def shard_json_file(input_file, output_directory, shard_size):
    with open(input_file, 'r', encoding='utf-8') as f:
        data = json.load(f)

    total_items = len(data)
    num_shards = (total_items + shard_size - 1) // shard_size

    for shard_number in range(num_shards):
        start_idx = shard_number * shard_size
        end_idx = min((shard_number + 1) * shard_size, total_items)
        shard_data = data[start_idx:end_idx]

        output_filename = os.path.join(output_directory, f"shard{shard_number}.json")
        with open(output_filename, 'w', encoding='utf-8') as f_out:
            json.dump(shard_data, f_out, indent=2, ensure_ascii=False)

        print(f"Shard {shard_number} created. Size: {len(shard_data)} items.")

if __name__ == "__main__":
    input_file = "/home/xdoestech/llama2.c/data/1Mgpt4/data.json"
    output_directory = "/home/xdoestech/qlora/llm_qlora/data"
    shard_size = 1000  # Adjust the shard size as needed

    shard_json_file(input_file, output_directory, shard_size)
