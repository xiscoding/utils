from datasets import load_dataset
from datasets.dataset_dict import DatasetDict

from data_processor.DataProcessor import DataProcessor


class OrcaDataProcessor(DataProcessor):
    def __init__(self, config, tokenizer):
        self.config = config
        self.tokenizer = tokenizer

    # def get_data(self) -> DatasetDict:
    #     if "model_context_window" in self.config:
    #         context_window = self.config["model_context_window"]
    #     else:
    #         context_window = self.tokenizer.model_max_length

    #     data = load_dataset(self.config["data"]["dataset"])
    #     data = data.map(lambda data_point: self.tokenizer(
    #         self._generate_prompt(
    #             data_point["conversations"],
    #             self.tokenizer.eos_token),
    #         max_length=context_window,
    #         truncation=True,
    #     ))
    #     return data

    ###GPT ADDITION 8/7
    def get_data(self) -> DatasetDict:
        if "model_context_window" in self.config:
            context_window = self.config["model_context_window"]
        else:
            context_window = self.tokenizer.model_max_length

        data = load_dataset(self.config["data"]["dataset"])
        data = data.map(lambda data_point: self.tokenizer(
            self._generate_prompt(
                data_point["system_prompt"], 
                data_point["question"], 
                data_point["response"], 
                self.tokenizer.eos_token),
            max_length=context_window,
            truncation=True,
        ))
        return data
    
    # def _generate_prompt(self, convo: list, eos_token: str) -> str:
    #     convo_text = ""
    #     for turn in convo:
    #         entity = turn["from"]
    #         value = turn["value"]

    #         if entity == "human":
    #             convo_text += "### HUMAN:\n"
    #             end_token = ""
    #         elif entity == "gpt":
    #             convo_text += "### RESPONSE:\n"
    #             end_token = eos_token  # LLM should stop its output after the response
    #         else:
    #             print(f"WARNING: uknown entity {entity}")
    #             convo_text += f"### {entity.upper()}:\n"
    #             end_token = ""

    #         convo_text += value + end_token + "\n\n"
    #     return convo_text
    
    ##GPT ADDITION 8/7  
    def _generate_prompt(self, system_prompt: str, question: str, response: str, eos_token: str) -> str:
        prompt_text = ""
        
        prompt_text += "### SYSTEM PROMPT:\n"
        prompt_text += system_prompt + "\n\n"
        
        prompt_text += "### QUESTION:\n"
        prompt_text += question + "\n\n"
        
        prompt_text += "### RESPONSE:\n"
        prompt_text += response + eos_token + "\n\n"
        
        return prompt_text
