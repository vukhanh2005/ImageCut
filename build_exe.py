import os
import sys
import PyInstaller.__main__

def build_executable():
    print("Building standalone Windows Executable using PyInstaller...")
    
    project_root = os.path.dirname(os.path.abspath(__file__))
    main_script = os.path.join(project_root, "main.py")
    
    args = [
        main_script,
        "--name=BackgroundRemover",
        "--onefile",
        "--windowed", # No console window
        f"--add-data={os.path.join(project_root, 'app')};app",
        "--collect-all=PySide6",
        "--collect-all=onnxruntime",
        "--hidden-import=PIL",
        "--hidden-import=cv2",
        "--hidden-import=scipy",
        "--clean"
    ]
    
    print(f"Executing PyInstaller with args: {args}")
    PyInstaller.__main__.run(args)
    print("Build complete! Executable is located in the 'dist' directory.")

if __name__ == "__main__":
    build_executable()
