# Setup Qt 6 C++ headers into Cpp/third_party/qt_include

$qtExtracted = "Cpp/third_party/qtbase_extracted/qtbase-6.7.2"
$targetInc = "Cpp/third_party/qt_include"

if (Test-Path $qtExtracted) {
    echo "Structuring Qt 6 C++ headers..."
    New-Item -ItemType Directory -Force -Path $targetInc

    # Copy src/corelib headers -> QtCore
    New-Item -ItemType Directory -Force -Path "$targetInc/QtCore"
    Copy-Item -Path "$qtExtracted/src/corelib/*" -Destination "$targetInc/QtCore" -Recurse -Force

    # Copy src/gui headers -> QtGui
    New-Item -ItemType Directory -Force -Path "$targetInc/QtGui"
    Copy-Item -Path "$qtExtracted/src/gui/*" -Destination "$targetInc/QtGui" -Recurse -Force

    # Copy src/widgets headers -> QtWidgets
    New-Item -ItemType Directory -Force -Path "$targetInc/QtWidgets"
    Copy-Item -Path "$qtExtracted/src/widgets/*" -Destination "$targetInc/QtWidgets" -Recurse -Force

    echo "Qt 6 C++ headers successfully structured in Cpp/third_party/qt_include!"
} else {
    echo "Qt 6 extraction pending..."
}
