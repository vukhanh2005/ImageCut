# Generate Qt 6 Include Headers Structure cleanly with ASCII encoding

$qtSrc = "Cpp/third_party/qtbase_extracted/qtbase-6.7.2/src"
$targetBase = "Cpp/third_party/include"

function ProcessModule($moduleName, $subDir) {
    $files = Get-ChildItem -Path "$qtSrc/$moduleName" -Recurse -Filter "q*.h"
    foreach ($f in $files) {
        if ($f.Name.EndsWith("_p.h")) { continue }
        
        # Copy header file to module dir
        Copy-Item -Path $f.FullName -Destination "$targetBase/$subDir/$($f.Name)" -Force
        
        # Determine CamelCase name
        $raw = $f.BaseName
        $camel = "Q" + $raw.Substring(1)
        
        # Create CamelCase forwarder header
        $incLine = "#include <$subDir/$($f.Name)>`n"
        [System.IO.File]::WriteAllText("$targetBase/$subDir/$camel", $incLine, [System.Text.Encoding]::ASCII)
        [System.IO.File]::WriteAllText("$targetBase/$camel", $incLine, [System.Text.Encoding]::ASCII)
    }
}

ProcessModule "corelib" "QtCore"
ProcessModule "gui" "QtGui"
ProcessModule "widgets" "QtWidgets"

# Additional global headers needed by Qt
$globalHeaders = Get-ChildItem -Path "$qtSrc/corelib/global" -Filter "*.h"
foreach ($g in $globalHeaders) {
    Copy-Item -Path $g.FullName -Destination "$targetBase/QtCore/$($g.Name)" -Force
    Copy-Item -Path $g.FullName -Destination "$targetBase/$($g.Name)" -Force
}

echo "Qt 6 Include headers clean generation done!"
