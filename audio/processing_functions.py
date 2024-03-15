import os
import shutil
from tqdm import tqdm #for loops, wrap executable 
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.io import wavfile
from python_speech_features import mfcc, logfbank
import librosa

def plot_signals(signals):
    fig, axes = plt.subplots(nrows=2, ncols=5, sharex=False,
                             sharey=True, figsize=(20,5))
    fig.suptitle('Time Series', size=16)
    i = 0
    for x in range(2):
        for y in range(5):
            axes[x,y].set_title(list(signals.keys())[i])
            axes[x,y].plot(list(signals.values())[i])
            axes[x,y].get_xaxis().set_visible(False)
            axes[x,y].get_yaxis().set_visible(False)
            i += 1

def plot_fft(fft):
    fig, axes = plt.subplots(nrows=2, ncols=5, sharex=False,
                             sharey=True, figsize=(20,5))
    fig.suptitle('Fourier Transforms', size=16)
    i = 0
    for x in range(2):
        for y in range(5):
            data = list(fft.values())[i]
            Y, freq = data[0], data[1]
            axes[x,y].set_title(list(fft.keys())[i])
            axes[x,y].plot(freq, Y)
            axes[x,y].get_xaxis().set_visible(False)
            axes[x,y].get_yaxis().set_visible(False)
            i += 1

def plot_fbank(fbank):
    fig, axes = plt.subplots(nrows=2, ncols=5, sharex=False,
                             sharey=True, figsize=(20,5))
    fig.suptitle('Filter Bank Coefficients', size=16)
    i = 0
    for x in range(2):
        for y in range(5):
            axes[x,y].set_title(list(fbank.keys())[i])
            axes[x,y].imshow(list(fbank.values())[i],
                    cmap='hot', interpolation='nearest')
            axes[x,y].get_xaxis().set_visible(False)
            axes[x,y].get_yaxis().set_visible(False)
            i += 1

def plot_mfccs(mfccs):
    fig, axes = plt.subplots(nrows=2, ncols=5, sharex=False,
                             sharey=True, figsize=(20,5))
    fig.suptitle('Mel Frequency Cepstrum Coefficients', size=16)
    i = 0
    for x in range(2):
        for y in range(5):
            axes[x,y].set_title(list(mfccs.keys())[i])
            axes[x,y].imshow(list(mfccs.values())[i],
                    cmap='hot', interpolation='nearest')
            axes[x,y].get_xaxis().set_visible(False)
            axes[x,y].get_yaxis().set_visible(False)
            i += 1

def envelope(y, rate, threshold):
  mask =[]
  y =pd.Series(y).apply(np.abs)
  y_mean = y.rolling(window=int(rate/10), min_periods=1, center=True).mean()
  for mean in y_mean:
    if mean > threshold:
      mask.append(True)
    else:
      mask.append(False)
  return mask

def calc_fft(y, rate):
  n = len(y)
  freq = np.fft.rfftfreq(n, d=1/rate)
  Y = abs(np.fft.rfft(y)/n)
  return (Y, freq)

def display_bar_graph(data, title="", x_label="", y_label=""):
  """
  Plots a bar graph with customized axis labels and title.

  Args:
    data: A dictionary or list containing the data points for each bar.
    title: The title of the plot (optional).
    x_label: The label for the x-axis (optional).
    y_label: The label for the y-axis (optional).
  """
  
  # Extract labels and values from data
  labels = list(data.keys()) if isinstance(data, dict) else range(len(data))
  values = list(data.values()) if isinstance(data, dict) else data

  # Create the bar plot
  plt.figure(figsize=(8, 6))  # Adjust figure size as needed
  plt.bar(labels, values, color='skyblue', ec='k')
  
  # Set axis limits and labels
  plt.xticks(range(len(labels)), labels, rotation=45, ha='right')  # Rotate x-axis labels for better readability
  plt.yticks(range(0, 51, 5), [f"{i}%" for i in range(0, 51, 5)])  # Add percentage labels to y-axis ticks

  # Set plot title and labels (if provided)
  if title:
    plt.title(title)
  if x_label:
    plt.xlabel(x_label)
  if y_label:
    plt.ylabel(y_label)

  # Display the plot
  plt.grid(axis='y', linestyle='--', alpha=0.7)  # Add grid lines for better readability
  plt.tight_layout()
  plt.show()

# -- Plot class distribution using a bar graph --
def plot_class_distribution_bar(df):
    """Plots the class distribution as a bar graph."""
    classes = list(np.unique(df.label))
    class_dist = df.groupby(['label'])['length'].mean()

    # Convert distribution to a dictionary for the bar graph function
    class_dist_dict = dict(class_dist)

    # Display the bar graph
    display_bar_graph(class_dist_dict, title="Class Distribution", x_label="Classes", y_label="Average Length (Percentage)")

def plot_class_distribution(df):
    """Plots the class distribution of the dataset."""
    classes = list(np.unique(df.label))  # Extract unique classes
    class_dist = df.groupby(['label'])['length'].mean()  # Calculate average length per class

    # Create the pie chart
    fig, ax = plt.subplots()
    ax.set_title('Class Distribution', y=1.08)
    ax.pie(class_dist, labels=class_dist.index, autopct='%1.1f%%',
           shadow=False, startangle=90)
    ax.axis('equal')
    plt.show()

def organize_audio_files(data_frame, label_column_name, source_folder, target_folder):
  """
  Organizes audio files based on their labels in the specified column of a pandas DataFrame.

  Args:
      data_frame: The pandas DataFrame containing information about the audio files.
      label_column_name: The name of the column containing the label for each audio file.
      target_folder: The name of the folder to create for organizing the files.
  """
  # Create the target folder if it doesn't exist
  if not os.path.exists(target_folder):
    os.makedirs(target_folder)

  # Iterate through each row in the DataFrame
  for index, row in data_frame.iterrows():
    # Get the label and filename
    label = row[label_column_name]
    filename = row['fname']

    # Create a subfolder for the label if it doesn't exist
    label_folder = os.path.join(target_folder, label)
    if not os.path.exists(label_folder):
      os.makedirs(label_folder)

    # Move the file to the label subfolder
    source_path = os.path.join(source_folder, filename)
    destination_path = os.path.join(label_folder, filename)
    shutil.move(source_path, destination_path)

def clean_deadspots(df, source_folder, target_folder):
    if len(os.listdir(target_folder)) == 0:
        for f in tqdm(df.fname):
            signal, rate = librosa.load(os.path.join(source_folder, f),sr=16000)
            mask = envelope(signal, rate, 0.0005)
            wavfile.write(filename=os.path.join(target_folder, f), rate=rate, data=signal[mask])
    else:
       print(f'Target folder: {target_folder} is not empty')