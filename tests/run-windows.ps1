param([switch]$Run)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$msbuild = 'C:/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe'
$project = Join-Path $repo 'PaintCoreTests.generated.vcxproj'
if (Test-Path -LiteralPath $project) { throw "Temporary project already exists: $project" }
try {
    $xml = [System.IO.File]::ReadAllText((Join-Path $repo 'GlowingDrawingForWEB.vcxproj'))
    # The source project may already list this test. Replace main with exactly one copy.
    $xml = $xml.Replace('<ClCompile Include="tests\paintcore_tests.cpp" />', '')
    $xml = $xml.Replace('Include="src\main.cpp"', 'Include="tests\paintcore_tests.cpp"')
    $xml = $xml.Replace('<IntDir>obj\$(Platform)\$(Configuration)\</IntDir>', '<IntDir>obj\PaintCoreTests\</IntDir>')
    $xml = $xml.Replace('<TargetName>$(ProjectName)_debug</TargetName>', '<TargetName>PaintCoreTests</TargetName>')
    # Share the already-built oF library, but keep test object files separate.
    [System.IO.File]::WriteAllText($project, $xml)
    Push-Location $repo
    try {
        & $msbuild $project /p:Configuration=Debug /p:Platform=x64 /p:BuildProjectReferences=false /m /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) { throw 'Test build failed.' }
        if ($Run) {
            $process = Start-Process -FilePath (Join-Path $repo 'bin/PaintCoreTests.exe') -WorkingDirectory (Join-Path $repo 'bin') -WindowStyle Hidden -PassThru
            if (-not $process.WaitForExit(60000)) { $process.Kill(); throw 'Tests timed out.' }
            if ($process.ExitCode -ne 0) { throw "Tests failed, process exit code: $($process.ExitCode)" }
            $result = Get-Content (Join-Path $repo 'obj/paintcore-tests/paintcore-test-result.txt')
            $result
            if ($result -ne 'PASS') { throw 'Tests failed.' }
        }
    } finally { Pop-Location }
} finally {
    # Only the exact generated project above is removed.
    if (Test-Path -LiteralPath $project) { Remove-Item -LiteralPath $project }
}
