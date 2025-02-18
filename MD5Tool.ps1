Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$cwd = Get-Location
$form       = New-Object System.Windows.Forms.Form
$comboBox   = New-Object System.Windows.Forms.ComboBox
$buttonOpen = New-Object System.Windows.Forms.Button
$labelInfo  = New-Object System.Windows.Forms.Label
$buttonRun  = New-Object System.Windows.Forms.Button
$buttonStop = New-Object System.Windows.Forms.Button
$timer      = New-Object System.Windows.Forms.Timer

function SetRegistoryString
{
    Param
    (
        [string]$path,
        [string]$name,
        [string]$value
    )
    $ErrorActionPreference = "silentlycontinue"
    New-Item         -Path $path
    New-ItemProperty -Path $path -Name $name -PropertyType 'String' -Value $value
    Set-ItemProperty -Path $path -Name $name                        -Value $value
    $ErrorActionPreference = "continue"
}

function SetRegistoryDword
{
    Param
    (
        [string]$path,
        [string]$name,
        [int]$value
    )
    $ErrorActionPreference = "silentlycontinue"
    New-Item         -Path $path
    New-ItemProperty -Path $path -Name $name -PropertyType 'DWord' -Value $value
    Set-ItemProperty -Path $path -Name $name                       -Value $value
    $ErrorActionPreference = "continue"
}

function buttonOpen_Click()
{
    param
    (
        [System.Object]$senderFrom,
        [System.EventArgs]$e
    )
    $buttonOpen.Enabled = $False
    $form.Refresh()
    $dialog=New-Object System.Windows.Forms.FolderBrowserDialog
    $dialog.Description="Select Folder"
    if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK)
    {
        $comboBox.Text = $dialog.SelectedPath
    }
    $buttonOpen.Enabled = $True
    $form.Refresh()
}

function buttonStop_Click()
{
    param
    (
        [System.Object]$senderFrom,
        [System.EventArgs]$e
    )
    SetRegistoryDword "HKCU:\Software\MD5Tool" "Status" 4 # 4:中断要求
}

function buttonRun_Click()
{
    param
    (
        [System.Object]$senderFrom,
        [System.EventArgs]$e
    )
    $path = $comboBox.Text
    if (-Not (Test-Path $path))
    {
        [System.Windows.Forms.MessageBox]::Show($path + " is not found", "Error", "OK", "Error")
        return
    }
    $comboBox.Enabled = $False
    $buttonOpen.Enabled = $False
    $buttonStop.Enabled = $True
    $buttonRun.Enabled = $False
    $labelInfo.Text = ""
    $form.Refresh()
    SetRegistoryString "HKCU:\Software\MD5Tool" "FromPath" $path
    SetRegistoryDword  "HKCU:\Software\MD5Tool" "Status" 1 # 1:実行要求
    $timer.Enabled = $True
}

function timer_Tick()
{
    param
    (
        [System.Object]$senderFrom,
        [System.EventArgs]$e
    )
    $regStatus = Get-ItemProperty -Path "HKCU:\Software\MD5Tool" -Name "Status"
    if ($regStatus.Status -eq 2)    # 2:実行中
    {
        $regPath = Get-ItemProperty -Path "HKCU:\Software\MD5Tool" -Name "ToPath"
        $labelInfo.Text = $regPath.ToPath
        $form.Refresh()
    }
    if ($regStatus.Status -eq 3)    # 3:正常終了
    {
        $timer.Enabled = $False
        $comboBox.Enabled = $True
        $buttonOpen.Enabled = $True
        $buttonStop.Enabled = $False
        $buttonRun.Enabled = $True
        $labelInfo.Text = "Finished"
        $form.Refresh()
        SetRegistoryDword "HKCU:\Software\MD5Tool" "Status" 0 # 0:実行待ち
    }
    if ($regStatus.Status -eq 4)    # 4:中断要求
    {
        $timer.Enabled = $False
        $comboBox.Enabled = $True
        $buttonOpen.Enabled = $True
        $buttonStop.Enabled = $False
        $buttonRun.Enabled = $True
        $labelInfo.Text = "Stop"
        $form.Refresh()
        SetRegistoryDword "HKCU:\Software\MD5Tool" "Status" 0 # 0:実行待ち
    }
}

SetRegistoryDword "HKCU:\Software\MD5Tool" "Status" 0 # 0:実行待ち
$Font = New-Object System.Drawing.Font("ＭＳ ゴシック",12)

$form.Text = "MD5Tool"
$form.Size = New-Object System.Drawing.Size(520,200)
$form.MinimumSize = New-Object System.Drawing.Size(320,200)
$form.StartPosition = "CenterScreen"
$form.font = $Font

$comboBox.Location = New-Object System.Drawing.Point(100,10)
$comboBox.size = New-Object System.Drawing.Size(350,30)
$comboBox.Anchor = 1 + 4 + 8 # 1:Top + 4:Left + 8:Right
$comboBox.DropDownStyle = "DropDown"
$comboBox.FlatStyle = "standard"
$comboBox.font = $Font
$comboBox.TabIndex = 0

$comboBoxHeight = $comboBox.Size.Height

$label = New-Object System.Windows.Forms.Label
$label.Location = New-Object System.Drawing.Point(10,10)
$label.Size = New-Object System.Drawing.Size(90,$comboBoxHeight)
$label.Text = "Folder"
$label.TextAlign = 16 # 16:MiddleLeft
$form.Controls.Add($label)

$buttonOpen.Location = New-Object System.Drawing.Point(450,10)
$buttonOpen.Size = New-Object System.Drawing.Size(40,$comboBoxHeight)
$buttonOpen.Anchor = 1 + 8 # 1:Top + 8:Right
$buttonOpen.Text = "..."
$buttonOpen.Add_Click({param($s,$e) buttonOpen_Click $s $e})
$form.Controls.Add($buttonOpen)

$labelInfo.BorderStyle = 1 # 1:FixedSingle
$labelInfo.Location = New-Object System.Drawing.Point(10,40)
$labelInfo.Size = New-Object System.Drawing.Size(480,60)
$labelInfo.Anchor = 1 + 2 + 4 + 8 # 1:Top + 2:Bottom + 4:Left + 8:Right
$labelInfo.Text = ""
$form.Controls.Add($labelInfo)

$buttonStop.Location = New-Object System.Drawing.Point(280,120)
$buttonStop.Size = New-Object System.Drawing.Size(100,30)
$buttonStop.Anchor = 2 + 8 # 2:Bottom + 8:Right
$buttonStop.Text = "Stop"
$buttonStop.Enabled = $False
$buttonStop.Add_Click({param($s,$e) buttonStop_Click $s $e})
$form.Controls.Add($buttonStop)

$buttonRun.Location = New-Object System.Drawing.Point(390,120)
$buttonRun.Size = New-Object System.Drawing.Size(100,30)
$buttonRun.Anchor = 2 + 8 # 2:Bottom + 8:Right
$buttonRun.Text = "Run"
$buttonRun.Add_Click({param($s,$e) buttonRun_Click $s $e})
$form.Controls.Add($buttonRun)

$timer.Enabled = $False
$timer.Interval = 500
$timer.Add_Tick({param($s,$e) timer_Tick $s $e})

$xmlPath = $PSScriptRoot + "\md5tool.xml"
# xml 読込
# TIPS:StreamReader を使用しないと "他のプロセスが使用中" になる
$sr = [System.IO.StreamReader]::new([System.IO.FileStream]::new($xmlPath,
      [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read,
      [System.IO.FileShare]::ReadWrite + [System.IO.FileShare]::Delete))
$xmlTextReader = New-Object System.Xml.XmlTextReader($sr)
$isDirectory = $false
$directory = ""
while ($xmlTextReader.Read())
{
    if ($xmlTextReader.NodeType.Equals([System.Xml.XmlNodeType]::Element))
    {
        $isDirectory = $false
        if ($xmlTextReader.Name.Equals("Directory"))
        {
            $isDirectory = $true
        }
    }
    elseif ($xmlTextReader.NodeType.Equals([System.Xml.XmlNodeType]::Text))
    {
        if ($isDirectory)
        {
            $directory = $xmlTextReader.Value
        }
    }
}
$xmlTextReader.Close()
$xmlTextReader.Dispose()
$sr.Close()
$sr.Dispose()

# コンボボックスへ項目を設定
$i = 0
foreach ($path in $directory.Split("`n"))
{
    $path = $path.Trim()
    if ($path.Length -gt 0)
    {
        if ($i -eq 0)
        {
            $comboBox.Text = $path
        }
        [void]$comboBox.Items.Add($path)
        $i++
    }
}
$form.Controls.Add($comboBox)

$form.ShowDialog()

# コンボボックスの項目を xml に保存
$crlf = [char]13 + [char]10
$directory = $crlf
$text = $comboBox.Text.Trim()
if ($text.Length -gt 0)
{
    $directory += $text + $crlf
}
foreach ($item in $comboBox.Items)
{
    if ($item -ne $text)
    {
        $directory += $item + $crlf
    }
}

# TIPS:StreamWriter を使用しないと "他のプロセスが使用中" になる
$sw = New-Object System.IO.StreamWriter($xmlPath, $false, [System.Text.Encoding]::UTF8)
$xmlTextWriter = New-Object System.Xml.XmlTextWriter($sw)
$xmlTextWriter.WriteStartDocument()
$xmlTextWriter.WriteWhitespace($crlf)
$xmlTextWriter.WriteStartElement("Settings")
$xmlTextWriter.WriteString($crlf)
$xmlTextWriter.WriteElementString("Directory", $directory)
$xmlTextWriter.WriteWhitespace($crlf)
$xmlTextWriter.WriteEndElement()
$xmlTextWriter.WriteWhitespace($crlf)
$xmlTextWriter.WriteEndDocument()
$xmlTextWriter.Close()
$xmlTextWriter.Dispose()
$sw.Close()
$sw.Dispose()

Set-Location -Path $cwd
