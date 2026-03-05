"""
Training script for a tiny eye-state (awake vs drowsy) classifier.

This script is intentionally simple and focused on portability to
microcontrollers (via TensorFlow Lite Micro). It is designed to be
easy to read for an engineering audience and to serve as a portfolio
example rather than a production-grade pipeline.

Data assumptions
----------------
- Images are stored in a directory structure like:
    ml/data/awake/*.jpg
    ml/data/drowsy/*.jpg
- Images can be captured later from the ESP32-S3 Sense or another
  camera and labeled manually into these folders.

Model
-----
- Small CNN operating on grayscale images (e.g., 64x64).
- Output: two-class softmax (awake, drowsy).

Outputs
-------
- Trained Keras model (optional, e.g., .h5).
- TensorFlow Lite model (.tflite) suitable for conversion to a C array
  for use with TensorFlow Lite Micro on the ESP32-S3.
"""

import pathlib
from typing import Tuple

import numpy as np
from PIL import Image
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers


BASE_DIR = pathlib.Path(__file__).resolve().parent
DATA_DIR = BASE_DIR / "data"
MODELS_DIR = BASE_DIR / "models"

IMAGE_SIZE: Tuple[int, int] = (64, 64)
CLASS_NAMES = ["awake", "drowsy"]


def load_dataset() -> Tuple[np.ndarray, np.ndarray]:
    """Load images from ml/data into numpy arrays.

    This is a minimal example implementation. For real experiments you
    may want to add data augmentation, better train/val splits, and
    more robust error handling.
    """
    images = []
    labels = []

    for label_idx, class_name in enumerate(CLASS_NAMES):
        class_dir = DATA_DIR / class_name
        if not class_dir.exists():
            print(f"[WARN] Class directory not found: {class_dir}")
            continue

        for img_path in class_dir.glob("*.jpg"):
            try:
                with Image.open(img_path) as img:
                    img = img.convert("L")  # grayscale
                    img = img.resize(IMAGE_SIZE)
                    arr = np.array(img, dtype=np.float32) / 255.0
                    images.append(arr)
                    labels.append(label_idx)
            except Exception as exc:  # pylint: disable=broad-except
                print(f"[WARN] Skipping {img_path}: {exc}")

    if not images:
        raise RuntimeError(
            "No images loaded. Populate ml/data/awake and ml/data/drowsy "
            "with labeled images before running training."
        )

    x = np.stack(images, axis=0)
    y = np.array(labels, dtype=np.int32)

    # Add channel dimension: (N, H, W, 1)
    x = np.expand_dims(x, axis=-1)
    return x, y


def build_model(input_shape: Tuple[int, int, int]) -> keras.Model:
    """Build a small CNN suitable for microcontroller deployment."""
    inputs = keras.Input(shape=input_shape)

    x = layers.Conv2D(8, (3, 3), activation="relu", padding="same")(inputs)
    x = layers.MaxPooling2D((2, 2))(x)

    x = layers.Conv2D(16, (3, 3), activation="relu", padding="same")(x)
    x = layers.MaxPooling2D((2, 2))(x)

    x = layers.Conv2D(32, (3, 3), activation="relu", padding="same")(x)
    x = layers.MaxPooling2D((2, 2))(x)

    x = layers.Flatten()(x)
    x = layers.Dense(32, activation="relu")(x)
    outputs = layers.Dense(len(CLASS_NAMES), activation="softmax")(x)

    model = keras.Model(inputs=inputs, outputs=outputs, name="eye_state_cnn")
    model.compile(
        optimizer="adam",
        loss="sparse_categorical_crossentropy",
        metrics=["accuracy"],
    )
    return model


def train_and_export() -> None:
    """Train the model and export a TFLite version."""
    MODELS_DIR.mkdir(parents=True, exist_ok=True)

    print("[INFO] Loading dataset...")
    x, y = load_dataset()

    # Simple train/validation split
    num_samples = x.shape[0]
    indices = np.arange(num_samples)
    np.random.shuffle(indices)

    split = int(0.8 * num_samples)
    train_idx, val_idx = indices[:split], indices[split:]

    x_train, y_train = x[train_idx], y[train_idx]
    x_val, y_val = x[val_idx], y[val_idx]

    print(f"[INFO] Train samples: {x_train.shape[0]}, Val samples: {x_val.shape[0]}")

    model = build_model(input_shape=(IMAGE_SIZE[0], IMAGE_SIZE[1], 1))
    model.summary()

    callbacks = [
        keras.callbacks.EarlyStopping(
            monitor="val_accuracy", patience=5, restore_best_weights=True
        )
    ]

    history = model.fit(
        x_train,
        y_train,
        validation_data=(x_val, y_val),
        epochs=50,
        batch_size=32,
        callbacks=callbacks,
    )

    print("[INFO] Final validation accuracy:",
          float(history.history["val_accuracy"][-1]))

    keras_model_path = MODELS_DIR / "eye_state_cnn.h5"
    print(f"[INFO] Saving Keras model to {keras_model_path}")
    model.save(keras_model_path)

    # Convert to TFLite with basic optimizations to help with MCU deployment.
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    tflite_model = converter.convert()

    tflite_path = MODELS_DIR / "eye_state_cnn.tflite"
    print(f"[INFO] Saving TFLite model to {tflite_path}")
    with open(tflite_path, "wb") as f:
        f.write(tflite_model)

    print("[INFO] Training and export complete.")


if __name__ == "__main__":
    train_and_export()

