# アセンブリのロード
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$cwd = Get-Location
function Get-ChildFilePath
{
    [CmdletBinding()]
    Param
    (
        [Parameter(Mandatory,ValueFromPipeline)]
        [string]$path,
        [string]$notExist,
        [string]$fileList,
        [string]$stdout
    )
    Begin{}
    Process
    {
        #/L   リストのみ - いずれのファイルにも、コピーを実施しません。
        #/E   空のディレクトリを含むサブディレクトリをコピーします。
        #/B   バックアップ モードでファイルをコピーします。
        #/NP  進行状況なし - コピーの完了率を表示しません。
        #/FP  出力にファイルの完全なパス名を含めます。
        #/NS  サイズなし - ファイル サイズをログに記録しません。
        #/NC  クラスなし - ファイル クラスをログに記録しません。
        #/NDL ディレクトリなし - ディレクトリ名をログに記録しません。
        #/NJH ジョブ ヘッダーがありません。
        #/NJS ジョブ要約がありません。
        robocopy.exe $path $notExist /L /E /B /NP /FP /NS /NC /NJH /NJS /NDL /UNILOG:$fileList >$stdout
    }
}

# フォームを作成
$form = New-Object System.Windows.Forms.Form
# コンボボックスを作成
$comboBox = New-Object System.Windows.Forms.ComboBox
# ...開くボタンを作成
$buttonOpen = New-Object System.Windows.Forms.Button
# プログレスバーを作成
$progressBar = New-Object System.Windows.Forms.ProgressBar
# 情報ラベルを作成
$labelInfo = New-Object System.Windows.Forms.Label
# 実行ボタンを作成
$buttonRun = New-Object System.Windows.Forms.Button

function Get-File
{
    Param
    (
        [string]$path,
        [string]$hashList
    )
    if ($path.Length -gt 0)
    {
        $hash  = Get-FileHash -Algorithm MD5 -Path $path
        $value = $hash.Hash.ToLower() + "`t" + $path
        Add-Content -Encoding utf8 -Path $hashList -Value $value
        $labelInfo.Text = $path
        $labelInfo.Refresh()
    }
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
    #フォルダ参照ダイアログインスタンス生成
    $dialog=New-Object System.Windows.Forms.FolderBrowserDialog
    #ダイアログの説明表示
    $dialog.Description="フォルダ選択"
    #ダイアログ表示
    if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK)
    {
        $comboBox.Text = $dialog.SelectedPath
    }
    $buttonOpen.Enabled = $True
    $form.Refresh()
}

function buttonRun_Click()
{
    param
    (
        [System.Object]$senderFrom,
        [System.EventArgs]$e
    )
    $buttonRun.Enabled = $False
    $form.Refresh()
    $desktop  = [System.Environment]::GetFolderPath("Desktop")
    $getDate  = Get-Date -Format "yyyyMMdd_HHmmss"
    $stdout   = $desktop  + "\cout_" + $getDate
    $notExist = $desktop  + "\list_" + $getDate
    $fileList = $notExist + ".txt"
    $hashList = $desktop  + "\hash_" + $getDate + ".txt"
    while (Test-Path $notExist)
    {
        $notExist += Get-Random $(48..57+97..122) | % {[char]$_}
    }
    $path = $comboBox.Text
    if (Test-Path $path)
    {
        Get-ChildFilePath $Path $notExist $fileList $stdout
        $progressBar.Maximum = (Get-Content -Path $fileList).Length
        $progressBar.Value = 0
        $progressBar.Refresh()
        foreach ($file in Get-Content -Path $fileList)
        {
            Get-File $file.Trim() $hashList
            $progressBar.Value++
            $progressBar.Refresh()
        }
        $progressBar.Value = $progressBar.Maximum
        $progressBar.Refresh()
    }
    $buttonRun.Enabled = $True
    $labelInfo.Text = "終了しました"
    $form.Refresh()
}

# フォントの指定
$Font = New-Object System.Drawing.Font("ＭＳ ゴシック",12)

# フォーム全体の設定
$form.Text = "MD5Tool"
$form.Size = New-Object System.Drawing.Size(320,200)
$form.MinimumSize = New-Object System.Drawing.Size(320,200)
$form.StartPosition = "CenterScreen"
$form.font = $Font

# コンボボックスを初期設定
$comboBox.Location = New-Object System.Drawing.Point(100,10)
$comboBox.size = New-Object System.Drawing.Size(150,30)
$comboBox.Anchor = 1 + 4 + 8 #Top + Left + Right
$comboBox.DropDownStyle = "DropDown"
$comboBox.FlatStyle = "standard"
$comboBox.font = $Font
$comboBox.TabIndex = 0

$comboBoxHeight = $comboBox.Size.Height

# ラベルを表示
$label = New-Object System.Windows.Forms.Label
$label.Location = New-Object System.Drawing.Point(10,10)
$label.Size = New-Object System.Drawing.Size(90,$comboBoxHeight)
$label.Text = "Folder"
$label.TextAlign = 16 #MiddleLeft
$form.Controls.Add($label)

# ...開くボタンの設定
$buttonOpen.Location = New-Object System.Drawing.Point(250,10)
$buttonOpen.Size = New-Object System.Drawing.Size(40,$comboBoxHeight)
$buttonOpen.Anchor = 1 + 8 #Top + Right
$buttonOpen.Text = "..."
$buttonOpen.Add_Click({param($s,$e) buttonOpen_click $s $e})
$form.Controls.Add($buttonOpen)

# プログレスバーの設定
$progressBar.Location = New-Object System.Drawing.Point(10,40)
$progressBar.Size = New-Object System.Drawing.Size(280,30)
$progressBar.Anchor = 1 + 4 + 8 #Top + Left + Right
$form.Controls.Add($progressBar)

# 情報ラベルの設定
$labelInfo.Location = New-Object System.Drawing.Point(10,80)
$labelInfo.Size = New-Object System.Drawing.Size(280,30)
$labelInfo.Anchor = 1 + 4 + 8 #Top + Left + Right
$labelInfo.Text = ""
$labelInfo.TextAlign = 16 #MiddleLeft
$form.Controls.Add($labelInfo)

# 実行ボタンの設定
$buttonRun.Location = New-Object System.Drawing.Point(190,120)
$buttonRun.Size = New-Object System.Drawing.Size(100,30)
$buttonRun.Anchor = 1 + 8 #Top + Right
$buttonRun.Text = "実行"
$buttonRun.Add_Click({param($s,$e) buttonRun_click $s $e})
$form.Controls.Add($buttonRun)

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

# フォームにコンボボックスを追加
$form.Controls.Add($comboBox)

# フォームを最前面に表示
$form.Topmost = $True
# フォームを表示
$form.ShowDialog()

# コンボボックスの項目を xml に保存
$directory = "`n"
$text = $comboBox.Text.Trim()
if ($text.Length -gt 0)
{
    $directory += $text + "`n"
}
foreach ($item in $comboBox.Items)
{
    if ($item -ne $text)
    {
        $directory += $item + "`n"
    }
}

# TIPS:StreamWriter を使用しないと "他のプロセスが使用中" になる
$sw = New-Object System.IO.StreamWriter($xmlPath, $false, [System.Text.Encoding]::UTF8)
$xmlTextWriter = New-Object System.Xml.XmlTextWriter($sw)
$xmlTextWriter.WriteStartDocument()
$xmlTextWriter.WriteWhitespace([char]13 + [char]10)
$xmlTextWriter.WriteStartElement("Settings")
$xmlTextWriter.WriteString([char]13 + [char]10)
$xmlTextWriter.WriteElementString("Directory", $directory)
$xmlTextWriter.WriteWhitespace([char]13 + [char]10)
$xmlTextWriter.WriteEndElement()
$xmlTextWriter.WriteWhitespace([char]13 + [char]10)
$xmlTextWriter.WriteEndDocument()
$xmlTextWriter.Close()
$xmlTextWriter.Dispose()
$sw.Close()
$sw.Dispose()

Set-Location -Path $cwd
