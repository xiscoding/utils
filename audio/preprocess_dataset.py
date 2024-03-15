from processing_functions import organize_audio_files, clean_deadspots, pd

FILE_DIR= '/home/xdoestech/Desktop/dcc_music/FSDKaggle2018/FSDKaggle2018.audio_train'
DATASET_LABEL_MAP_CSV = '/home/xdoestech/Desktop/dcc_music/FSDKaggle2018/FSDKaggle2018.meta/train_post_competition.csv'
df = pd.read_csv(DATASET_LABEL_MAP_CSV)
source_folder = '/home/xdoestech/Desktop/dcc_music/FSDKaggle2018/FSDKaggle2018.audio_train/'
target_folder = '/home/xdoestech/Desktop/dcc_music/cleaned_audio_files/FSDKaggle2018/'

"""
preprocess: clean deadspots in dataset, organize audio into folders with sections.
"""
def preprocess_dataset(file_dir, source_folder, target_folder):
    clean_deadspots(df, source_folder, target_folder) # creates target folder
    organize_audio_files(df, 'label', target_folder, target_folder) # adds folders to target folder

if  __name__ == "__main__":
    preprocess_dataset(FILE_DIR, source_folder, target_folder)