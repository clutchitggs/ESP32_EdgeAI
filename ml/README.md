ML Pipeline for Eye-State Classification
=======================================

This directory contains the Python code used to train the tiny
eye-state classifier that will run on the ESP32-S3 as part of the
drowsiness detector.

## Directory layout

- `data/`
  - Placeholder directory for training images.
  - Expected structure:
    - `data/awake/*.jpg`
    - `data/drowsy/*.jpg`
- `models/`
  - Outputs from training (`.h5` and `.tflite` files).
- `requirements.txt`
  - Python dependencies for the training environment.
- `train_eye_state_model.py`
  - Script that loads the dataset, trains a small CNN, and exports a
    TensorFlow Lite model.

## Dataset

Initially, the script expects images organized as:

- `ml/data/awake/*.jpg`  – eyes open / awake state.
- `ml/data/drowsy/*.jpg` – eyes closed or clearly drowsy state.

You can:

- Start with a small public eye-open/eye-closed dataset (document the
  source in your own notes), or
- Capture images of yourself using a camera (ESP32-S3 Sense or webcam)
  and manually label them into the corresponding folders.

For best results on the ESP32-S3:

- Capture images in similar lighting and distance as your actual
  study setup.
- Keep backgrounds simple if possible.

## Usage

1. Create and activate a Python virtual environment (recommended).
2. Install dependencies from this directory:

   ```bash
   pip install -r requirements.txt
   ```

3. Populate the `data/awake` and `data/drowsy` directories with
   labeled images.
4. Run the training script:

   ```bash
   python train_eye_state_model.py
   ```

5. After training completes, you should see:

   - `models/eye_state_cnn.h5` – full Keras model (for inspection).
   - `models/eye_state_cnn.tflite` – quantized model suitable for
     microcontroller deployment.

## Integration into firmware

In a later stage, the `.tflite` model will be converted into a C array
and compiled into the ESP32-S3 firmware. The firmware will:

- Capture a small region of interest containing the eyes.
- Resize and normalize it to the input size expected by the model
  (e.g., 64x64 grayscale).
- Run the model on-device using TensorFlow Lite Micro (or a similar
  runtime).
- Map the model’s outputs to the `STATE_AWAKE` / `STATE_DROWSY` enum
  and feed that into the drowsiness decision logic.

The exact conversion and integration steps will be documented alongside
the firmware once the model is ready.

