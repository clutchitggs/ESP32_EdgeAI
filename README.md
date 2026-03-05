ESP32-S3 Drowsiness Detector
============================

An edge-AI project for the ESP32-S3 Sense that monitors eye state using the on-board camera to detect when a student is getting drowsy at their desk and triggers an alert (e.g., buzzer or LED). All inference runs fully on the ESP32-S3; no cloud is required.

## Project overview

This project is designed as a **portfolio-grade** example to showcase:

- Embedded systems skills (ESP32-S3, camera bring-up, GPIO, basic real-time loops).
- Edge AI and tinyML (small CNN running on-device for eye-state classification).
- Clean, professional firmware and ML code structure.
- Clear documentation suitable for recruiters and engineers reviewing a GitHub profile.

The primary use case is a low-cost, personal drowsiness monitor for students studying at a desk. The same architecture can be adapted to other domains such as driver monitoring, operator safety, or focused-work tracking.

## High-level architecture

At a high level, the system runs the following loop on the ESP32-S3 Sense:

1. Capture an image frame from the on-board camera.
2. Preprocess the frame to extract a small region of interest (eyes/upper face) and resize/normalize it.
3. Run a tiny convolutional neural network (CNN) on-device to classify the eye state as:
   - `awake` (eyes open)
   - `drowsy` (eyes mostly or fully closed)
4. Accumulate results over multiple frames with simple decision logic.
5. Trigger an alert (buzzer/LED) when drowsiness persists beyond a configurable threshold.
6. Optionally log events or expose basic status over serial or Wi-Fi in future extensions.

Conceptual data flow:

```mermaid
flowchart LR
    cameraSensor[Camera Sensor] --> frameBuffer[Frame Buffer]
    frameBuffer --> preproc[Preprocessing\n(ROI, resize, normalize)]
    preproc --> tinyCnn[Tiny CNN\n(eye state model)]
    tinyCnn --> decisionLogic[Drowsiness Decision\n(multi-frame)]
    decisionLogic --> alertOutput[Alert Output\n(buzzer/LED)]
```

The ML model is trained offline (Python/TensorFlow) and then exported in a microcontroller-friendly format (e.g., TensorFlow Lite Micro). The compiled model data is linked into the firmware and evaluated on-device.

## Repository layout

- `firmware/`
  - ESP-IDF style C/C++ firmware for the ESP32-S3 Sense.
  - Contains the main application, camera initialization, inference interface, and alert logic.
- `ml/`
  - Python code for data handling and model training.
  - Includes scripts or notebooks to train a small eye-state classifier and export it to an embedded-friendly format.
  - `data/` (placeholder) for training images.
  - `models/` (placeholder) for trained/exported models.
- `docs/`
  - Additional documentation, diagrams, and notes (optional; some documentation is kept directly in this README).

This structure is intentionally close to what a real embedded/ML project might use in industry.

## Build and run overview

### Firmware (ESP32-S3, ESP-IDF)

The firmware is intended to be built with **ESP-IDF** using an ESP32-S3 Sense board:

1. Install ESP-IDF following Espressif's official documentation.
2. Connect the ESP32-S3 Sense via USB.
3. From the `firmware/` directory:
   - Configure the project as needed (e.g., `idf.py menuconfig`).
   - Build: `idf.py build`
   - Flash: `idf.py -p <PORT> flash`
   - Monitor: `idf.py -p <PORT> monitor`

Once flashed, the device will:

- Initialize the camera.
- Capture frames at a modest resolution appropriate for the tinyML model.
- Run the (stubbed, then real) inference pipeline.
- Apply drowsiness decision logic and, when fully implemented, drive alert outputs.

> Note: Until the ML model is trained and integrated, the inference step will be a placeholder that always reports an "awake" state. This keeps the firmware compilable while the ML pipeline is developed.

### ML training pipeline (Python)

The ML side is developed and run on a host machine (e.g., your Windows PC with Python installed):

1. Create and activate a Python virtual environment (recommended).
2. From the `ml/` directory, install dependencies:
   - `pip install -r requirements.txt`
3. Prepare a dataset of eye-region images labeled as `awake` and `drowsy` (either using a small public dataset or by capturing and labeling your own images).
4. Run the training script or notebook (to be provided in `ml/`) to:
   - Load and preprocess the images.
   - Train a small CNN suitable for ESP32-S3.
   - Evaluate performance.
   - Export a TensorFlow Lite (or similar) model file.
5. Convert the exported model into a C array and include it in the firmware (dedicated helper code and instructions will be added).

The README and `ml/` documentation will describe the recommended workflow once the training script is implemented.

## Status and roadmap

Current status:

- Repository structure created (`firmware/`, `ml`, `docs`).
- Top-level README and MIT license in place.
- ESP-IDF firmware skeleton implemented:
  - `app_main()` entry point and FreeRTOS task loop.
  - Camera initialization and frame capture stubs (`camera.c` / `camera.h`).
  - Inference interface and drowsiness decision integration (`inference.c` / `inference.h`).
  - Alert output abstraction with a placeholder GPIO (`alert.c` / `alert.h`).
  - Model data placeholders (`model_data.c` / `model_data.h`).
- ML training pipeline scaffolding implemented:
  - `ml/requirements.txt` for Python dependencies.
  - `ml/train_eye_state_model.py` for training and exporting a tiny eye-state CNN.
  - `ml/README.md` documenting dataset expectations and usage.

Planned next steps:

1. **Data collection and training**
   - Capture and label eye-region images into `ml/data/awake` and `ml/data/drowsy`.
   - Run `train_eye_state_model.py` to train and export a `.tflite` model.
2. **Model integration on-device**
   - Convert the `.tflite` file to a C array and replace the placeholder in `model_data.c`.
   - Integrate a microcontroller inference runtime (e.g., TensorFlow Lite Micro) and connect it to `run_drowsiness_inference()`.
3. **Alert hardware and UX**
   - Connect a buzzer or LED to the configured GPIO and tune alert behavior.
   - Optionally add a simple serial or web-based status interface for debugging and demos.
4. **Testing and refinement**
   - Test the system in realistic study conditions.
   - Adjust thresholds, model, and preprocessing for robustness.

## How to demo (high-level)

In an interview or portfolio context, a typical demo flow could be:

1. Briefly explain the problem: students (and operators or drivers) get drowsy while working; this project is a low-cost, embedded drowsiness detector.
2. Show the ESP32-S3 Sense on your desk, connected via USB and pointed at your face.
3. Start the firmware and describe the pipeline:
   - Camera frames → preprocessing → tiny CNN → multi-frame decision → buzzer/LED alert.
4. Sit upright and look at your screen/book; the system should remain in the "awake" state (no alert).
5. Gradually close your eyes or simulate nodding off; after a short delay, the alert activates.
6. Briefly walk through the GitHub repo:
   - `firmware/` ESP-IDF structure and main loop.
   - `ml/` training pipeline and exported model.
   - The README architecture diagram and explanation.

This combination of a live demo and clear repository structure is intended to make a strong impression on embedded/edge-AI focused employers.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

