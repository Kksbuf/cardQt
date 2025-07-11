import sys
import traceback
import os
import json
import numpy as np
import cv2
from ultralytics import YOLO
from datetime import datetime
from queue import Queue, Empty
import threading
import time

# Global queue for detection requests
detection_queue = Queue()
processing_thread = None
should_stop = False

def process_queue(model):
    global should_stop
    while not should_stop:
        try:
            # Get item from queue with timeout
            try:
                image_path = detection_queue.get(timeout=1.0)
            except Empty:
                continue

            # Wait for the image to be fully written
            max_retries = 10
            retry_count = 0
            while retry_count < max_retries:
                if os.path.exists(image_path) and os.path.getsize(image_path) > 0:
                    # Add a small delay to ensure file is fully written
                    time.sleep(0.1)
                    break
                time.sleep(0.2)
                retry_count += 1

            if retry_count >= max_retries:
                print(f'[ERROR] Timeout waiting for image file: {image_path}')
                sys.stdout.flush()
                continue

            print(f'[STATUS] Processing image: {image_path}')
            sys.stdout.flush()
            
            detections, annotated_img = detect_defects(model, image_path)
            if detections is not None and annotated_img is not None:
                save_detection_results(image_path, detections, annotated_img)

            detection_queue.task_done()

        except Exception as e:
            print(f'[ERROR] Error in processing thread: {str(e)}')
            print('[ERROR] Traceback:')
            print(traceback.format_exc())
            sys.stdout.flush()

def initialize_model():
    try:
        print('[STATUS] Starting model initialization...')
        sys.stdout.flush()
        
        model_path = os.path.join(os.getcwd(), 'model', 'best.pt')
        print(f'[STATUS] Looking for model at: {model_path}')
        sys.stdout.flush()
        
        if not os.path.exists(model_path):
            print(f'[ERROR] Model not found at {model_path}')
            print('[ERROR] Please ensure the model file exists in the model directory')
            sys.stdout.flush()
            sys.exit(1)
        
        print('[STATUS] Found model file, loading YOLO...')
        sys.stdout.flush()
        
        model = YOLO(model_path)
        
        # Perform a warmup inference on a blank image
        print('[STATUS] Performing warmup inference...')
        sys.stdout.flush()
        
        # Create a small blank image for warmup
        warmup_img = np.zeros((640, 640, 3), dtype=np.uint8)
        warmup_path = os.path.join(os.getcwd(), 'warmup.jpg')
        cv2.imwrite(warmup_path, warmup_img)
        
        try:
            # Run inference on warmup image
            model(warmup_path)
            # Remove warmup image
            os.remove(warmup_path)
            print('[STATUS] Warmup inference completed')
        except Exception as we:
            print(f'[WARNING] Warmup inference failed: {str(we)}')
        finally:
            # Try to remove warmup image if it still exists
            if os.path.exists(warmup_path):
                try:
                    os.remove(warmup_path)
                except:
                    pass
        
        print('[SUCCESS] Model loaded successfully and ready for inference!')
        sys.stdout.flush()
        return model
    except Exception as e:
        print(f'[ERROR] Error initializing model: {str(e)}')
        print('[ERROR] Traceback:')
        print(traceback.format_exc())
        sys.stdout.flush()
        sys.exit(1)

def detect_defects(model, image_path):
    try:
        # Run detection
        results = model(image_path)
        result = results[0]  # Get first image result
        
        # Define class names
        class_names = ['damage', 'edge', 'mark', 'oil']
        
        # Process detections
        detections = []
        for box in result.boxes:
            x1, y1, x2, y2 = map(int, box.xyxy[0].tolist())  # Convert to integers
            width = x2 - x1
            height = y2 - y1
            class_id = int(box.cls)
            detection = {
                'xyxy': [x1, y1, x2, y2],
                'confidence': float(box.conf) * 100,  # Convert to percentage
                'class_id': class_id,
                'class_name': class_names[class_id],  # Use correct class name from list
                'width': width,
                'height': height,
                'area': width * height,
                'center_x': (x1 + x2) // 2,
                'center_y': (y1 + y2) // 2
            }
            detections.append(detection)
        
        # Use YOLO's built-in visualization
        annotated_img = result.plot()
        
        return detections, annotated_img
        
    except Exception as e:
        print(f'[ERROR] Error during detection: {str(e)}')
        print('[ERROR] Traceback:')
        print(traceback.format_exc())
        sys.stdout.flush()
        return None, None

def save_detection_results(image_path, detections, annotated_img):
    try:
        # Get the directory and base filename
        dir_path = os.path.dirname(image_path)
        base_name = os.path.splitext(os.path.basename(image_path))[0]
        
        # Save annotated image
        output_image_path = os.path.join(dir_path, f'{base_name}_detected.jpg')
        cv2.imwrite(output_image_path, annotated_img)
        
        # Save detection data
        detection_data = {
            'timestamp': datetime.now().isoformat(),
            'image_file': os.path.basename(image_path),
            'detections': detections
        }
        
        output_json_path = os.path.join(dir_path, f'{base_name}_detections.json')
        with open(output_json_path, 'w') as f:
            json.dump(detection_data, f, indent=2)
            
        print(f'[SUCCESS] Detection results saved for {os.path.basename(image_path)}')
        print(f'[INFO] Found {len(detections)} defects')
        for det in detections:
            print(f'[INFO] {det["class_name"]}: {det["confidence"]:.2f}')
        sys.stdout.flush()
        
        return True
        
    except Exception as e:
        print(f'[ERROR] Error saving detection results: {str(e)}')
        print('[ERROR] Traceback:')
        print(traceback.format_exc())
        sys.stdout.flush()
        return False

def process_command(model, command):
    try:
        cmd_parts = command.strip().split()
        if not cmd_parts:
            return
            
        cmd = cmd_parts[0]
        
        if cmd == 'detect':
            if len(cmd_parts) < 2:
                print('[ERROR] Missing image path for detect command')
                sys.stdout.flush()
                return
                
            image_path = ' '.join(cmd_parts[1:])  # Handle paths with spaces
            
            # Add to detection queue instead of processing immediately
            detection_queue.put(image_path)
            print(f'[STATUS] Queued image for detection: {image_path}')
            sys.stdout.flush()
            
        else:
            print(f'[ERROR] Unknown command: {cmd}')
            sys.stdout.flush()
            
    except Exception as e:
        print(f'[ERROR] Error processing command: {str(e)}')
        print('[ERROR] Traceback:')
        print(traceback.format_exc())
        sys.stdout.flush()

if __name__ == '__main__':
    print('[STATUS] Python script started')
    sys.stdout.flush()
    
    # Initialize model
    model = None
    while model is None:
        try:
            model = initialize_model()
        except Exception as e:
            print(f'[ERROR] Model initialization failed, retrying in 2 seconds...')
            print(f'[ERROR] Error: {str(e)}')
            sys.stdout.flush()
            import time
            time.sleep(2)
    
    # Start processing thread
    processing_thread = threading.Thread(target=process_queue, args=(model,))
    processing_thread.daemon = True
    processing_thread.start()
    
    print('[STATUS] Entering command loop')
    sys.stdout.flush()
    
    try:
        while True:
            line = sys.stdin.readline()
            if not line:
                break
            process_command(model, line)
    except Exception as e:
        print(f'[ERROR] Error in main loop: {str(e)}')
        print('[ERROR] Traceback:')
        print(traceback.format_exc())
        sys.stdout.flush()
    finally:
        # Clean shutdown
        should_stop = True
        if processing_thread:
            processing_thread.join(timeout=5.0) 