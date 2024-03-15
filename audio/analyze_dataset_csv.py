import os
from tqdm import tqdm #for loops, wrap executable 
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.io import wavfile
from python_speech_features import mfcc, logfbank
import librosa

from audio_classification.processing_functions import plot_class_distribution, plot_class_distribution_bar


DATASET_LABEL_MAP_CSV = '/home/xdoestech/Desktop/dcc_music/FSDKaggle2018/FSDKaggle2018.meta/train_post_competition.csv'
df = pd.read_csv(DATASET_LABEL_MAP_CSV)
df.set_index('fname', inplace=True)

for f in df.index:
  rate, signal = wavfile.read('/home/xdoestech/Desktop/dcc_music/FSDKaggle2018/FSDKaggle2018.audio_train/'+f)
  df.at[f, 'length'] = signal.shape[0]/rate

# Plot the class distribution using the organized function
plot_class_distribution(df.copy()) 
df.reset_index(inplace=True)
# Plot the class distribution using the bar graph function
plot_class_distribution_bar(df.copy())