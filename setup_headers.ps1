# Setup OpenCV headers into Cpp/third_party/include/opencv2

$extractedDir = "Cpp/third_party/opencv_extracted/opencv-4.10.0"
$targetInc = "Cpp/third_party/include"

if (Test-Path $extractedDir) {
    echo "Organizing OpenCV C++ headers..."
    New-Item -ItemType Directory -Force -Path $targetInc

    $modules = Get-ChildItem -Path "$extractedDir/modules"
    foreach ($m in $modules) {
        $incPath = "$extractedDir/modules/$($m.Name)/include/opencv2"
        if (Test-Path $incPath) {
            Copy-Item -Path $incPath -Destination $targetInc -Recurse -Force
        }
    }
    echo "OpenCV headers successfully structured in Cpp/third_party/include/opencv2!"
} else {
    echo "Extraction pending..."
}
