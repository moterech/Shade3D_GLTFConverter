# glTF Converter for Shade3D

Shade3DでglTF(拡張子 gltf/glb)をインポート/エクスポートするプラグインです。  
glTFの処理で、「Microsoft.glTF.CPP」( https://www.nuget.org/packages/Microsoft.glTF.CPP/ )を使用しています。

## 機能

以下の機能があります。

* glTF 2.0の拡張子「gltf」「glb」の3D形状ファイルをShade3Dに読み込み (インポート)
* Shade3DのシーンからglTF 2.0の「gltf」「glb」の3D形状ファイルを出力 （エクスポート）

## 動作環境

* Windows 7/8/10以降のOS  
* Shade3D ver.16/17以降で、Standard/Professional版（Basic版では動作しません）  
  Shade3Dの64bit版のみで使用できます。32bit版のShade3Dには対応していません。   

## 使い方

### プラグインダウンロード

以下から最新版をダウンロードしてください。  
https://github.com/ft-lab/Shade3D_GLTFConverter/releases

### プラグインを配置し、Shade3Dを起動

Windowsの場合は、ビルドされた glTFConverter64.dll をShade3Dのpluginsディレクトリに格納してShade3Dを起動。  
メインメニューの「ファイル」-「エクスポート」-「glTF(glb)」が表示されるのを確認します。  

### 使い方

Shade3Dでのシーン情報をエクスポートする場合、  
メインメニューの「ファイル」-「エクスポート」-「glTF(glb)」を選択し、指定のファイルにgltfまたはglb形式でファイル出力します。  
ファイルダイアログボックスでのファイル指定では、デフォルトではglb形式のバイナリで出力する指定になっています。   
ここで拡張子を「gltf」にすると、テキスト形式のgltfファイル、バイナリデータのbinファイル、テクスチャイメージの各種ファイルが出力されます。  

gltf/glbの拡張子の3DモデルデータをShade3Dのシーンにインポートする場合、   
メインメニューの「ファイル」-「インポート」-「glTF(gltf/glb)」を選択し、gltfまたはglbの拡張子のファイルを指定します。  

## サンプルファイル

サンプルのShade3Dのファイル(shd)、エクスポートしたglbファイルを https://ft-lab.github.io/gltf.html に置いています。    

## glTF入出力として対応している機能

* メッシュ情報の入出力
* シーンの階層構造の入出力
* マテリアル/テクスチャイメージとして、BaseColor(基本色)/Metallic(メタリック)/Roughness(荒さ)/Normal(法線)/Emissive(発光)/Occlusion(遮蔽)を入力
* マテリアル/テクスチャイメージとして、BaseColor(基本色)/Metallic(メタリック)/Roughness(荒さ)/Normal(法線)/Emissive(発光)を出力
* ボーンやスキン情報の入出力
* ポリゴンメッシュの頂点カラーの入出力 (頂点カラー0のみ) ver.0.1.0.3で追加。    
頂点カラーとしてRGBAの要素を使用しています。
* テクスチャマッピングの「アルファ透明」の入出力。ver.0.1.0.4で追加。  
* ボーンを使用したスキンアニメーション情報の入出力。ver.0.1.0.6で追加。   

PBR表現としては、metallic-roughnessマテリアルモデルとしてデータを格納しています。  

### インポート時のdoubleSidedのマテリアルについて (ver.0.1.0.11 - )

インポート時にdoubleSidedの属性を持つマテリアルは、    
Shade3Dのマスターサーフェス名に「_doubleSided」が付きます。    
なお、Shade3DのレンダリングではdoubleSidedでなくても両面が表示されることになります。    

### エクスポート時のdoubleSidedの対応について

glTFフォーマットでは、デフォルトでは表面のみ表示されます。   
裏面は非表示となります。   
裏面を表示させるには、表面材質に割り当てるマスターサーフェス名にて「doublesided」のテキストを含めると、その表面材質はdoubleSided対応として出力されます。

### エクスポート時の「アルファ透明」の対応について (ver.0.1.0.4 - )

表面材質のマッピングレイヤとして、「イメージ/拡散反射」の「アルファ透明」を選択した場合、    
元のイメージがpngのAlpha要素を持つ場合に、透過処理(glTFのalphaMode:MASK)が行われます。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_alpha_blend.png"/>     

この場合、エクスポート時のオプションで「テクスチャ出力」を「jpegに置き換え」としたときも、png形式で出力されます。

### エクスポート時のボーンとスキンについて

GLTF/GLBエクスポート時に、ボーンとスキンを持つ形状の場合は、その情報も出力されます。   
このとき、Shade3Dの「シーケンス Off」としての姿勢で出力されます。  
なお、ver.0.1.0.0ではシーケンス On時の姿勢はエクスポートで出力していません。   

スキンのバインド情報は、ポリゴンメッシュの1頂点に対して最大4つまでです。   

### エクスポート時の出力をスキップする形状について

ブラウザでレンダリングをしない指定にしてる場合、形状名の先頭に「#」を付けている場合は、その形状はエクスポートされません。

### インポート/エクスポート時のボーンについて

ボーンで、変換行列に回転/せん断/スケール要素を与えると正しくインポート/エクスポートされません。    
変換行列では移動要素のみ使用するようにしてください。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_bone_matrix.png"/>     
Shade3Dでボーンを扱う際は、「軸の整列」の「自動で軸合わせ」をオフにして使用するようにしてください。   
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_bone_matrix_2.png"/>     

### エクスポートオプション

エクスポート時にダイアログボックスが表示され、オプション指定できます。   
自由曲面や掃引体/回転体などの分割は「曲面の分割」で精度を指定できます。    
「テクスチャ出力」で「イメージから拡張子を参照」を選択すると、   
マスターイメージ名で「.png」の拡張子が指定されていればpngとして、   
「.jpg」の拡張子が指定されていればjpegとしてテクスチャイメージが出力されます。   
「テクスチャ出力」で「pngに置き換え」を選択すると、すべてのイメージはpngとして出力されます。   
「jpegに置き換え」を選択すると、すべてのイメージはjpgとして出力されます。   
ただし、テクスチャマッピングで「アルファ透明」を選択している場合は強制的にpng形式で出力されます。    
「ボーンとスキンを出力」チェックボックスをオンにすると、glTFファイルにボーンとスキンの情報を出力します。   
ファイルサイズを小さくしたい場合やポージングしたそのままの姿勢を出力したい場合はオフにします。   
「頂点カラーを出力」チェックボックスをオンにすると、ポリゴンメッシュに割り当てられた頂点カラー情報も出力します。    
「アニメーションを出力」チェックボックスをオンにすると、ボーン＋スキンのモーションを割り当てている場合にそのときのキーフレーム情報を出力します。    

「Asset情報」で主にOculus Homeにglbファイルを持っていくときの情報を指定できます(ver.0.1.0.8 追加)。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_export_dlg_asset.png"/>     
※ Oculus Homeでは全角の指定は文字化けしますので、半角英数字で指定する必要があります。    
「タイトル」で3Dモデルの名前を指定します。    
「作成者」で3Dモデルの作成者を指定します。    
「ライセンス選択」でライセンスの種類を、All rights reserved、Creative Commons、Public domain、その他、から指定します。    
その他の場合は、「ライセンス」に任意のテキストを入力できます。    
「ライセンス選択」で選択されたときの説明文が表示されますので、もっとも適するライセンス形態を選ぶようにします。    
「原型モデルの参照先」で、モデルデータを提供しているURLなどを指定します。    

ライセンスの種類は以下のものがあります。    

|ライセンスの種類|説明|    
|----|----|    
|All rights reserved|著作権を持ちます|   
|CC BY-4.0 ( https://creativecommons.org/licenses/by/4.0/ )|作品のクレジットを表示する必要あり。改変OK/営利OK。|   
|CC BY-SA-4.0 ( https://creativecommons.org/licenses/by-sa/4.0/ )|作品のクレジットを表示する必要あり。改変OK/営利OK/継承。|   
|CC BY-NC-4.0 ( https://creativecommons.org/licenses/by-nc/4.0/ )|作品のクレジットを表示する必要あり。改変OK/非営利。|   
|CC BY-ND-4.0 ( https://creativecommons.org/licenses/by-nd/4.0/ )|作品のクレジットを表示する必要あり。改変禁止/営利OK。|   
|CC BY-NC-SA-4.0 ( https://creativecommons.org/licenses/by-nc-sa/4.0/ )|作品のクレジットを表示する必要あり。改変OK/非営利/継承。|   
|CC BY-NC-ND-4.0 ( https://creativecommons.org/licenses/by-nc-nd/4.0/ )|作品のクレジットを表示する必要あり。改変禁止/非営利。|   
|CC0(Public domain) ( https://creativecommons.org/publicdomain/zero/1.0/ )|著作権を放棄します|   

「Creative Commons」については「 https://creativecommons.jp/licenses/ 」を参照してください。    
「Public domain」については「 https://creativecommons.org/publicdomain/zero/1.0/ 」を参照してください。    

### インポートオプション

インポート時にダイアログボックスが表示され、オプション指定できます。   
「イメージのガンマ補正」で1.0を選択すると、読み込むテクスチャイメージのガンマ補正はそのまま。   
1.0/2.2を選択すると、リニアワークフローを考慮した逆ガンマ値をテクスチャイメージに指定します。   
この逆ガンマを与えるテクスチャイメージは「BaseColor」「Emissive」のみです。    
「Roughness/Metallic」「Occlusion」「Normal」はガンマ補正を行わず、リニアとして読み込まれます。    
「アニメーションを読み込み」チェックボックスをオンにすると、ボーン＋スキンのモーション情報を読み込みます。    
「法線を読み込み」チェックボックスをオンにすると、「ベイクされる形」でポリゴンメッシュの法線が読み込まれます。   
Shade3Dではこれはオフにしたままのほうが都合がよいです。   
「限界角度」はポリゴンメッシュの限界角度値です。   
※ glTFには法線のパラメータはありますが、Shade3Dの限界角度に相当するパラメータは存在しません。   
「頂点カラーを読み込み」チェックボックスをオンにすると、メッシュに頂点カラーが割り当てられている場合はその情報も読み込まれます。    


### インポート時の表面材質の構成について

拡散反射色として、glTFのBaseColor(RGB)の値が反映されます。   
光沢1として、glTFのMetallic Factorの値が反映されます。このとき、0.3以下の場合は0.3となります。   
光沢1のサイズは0.7固定です。   
発光色として、glTFのEmissive Factorの値が反映されます。   
反射値として、glTFのMetallic Factorの値が反映されます。   
荒さ値として、glTFのRoughness Factorの値が反映されます。   

マッピングレイヤは以下の情報が格納されます。   
複雑化しているのは、PBR表現をなんとかShade3Dの表面材質でそれらしく似せようとしているためです。    

|glTFでのイメージの種類|マッピングレイヤ|合成|適用率|反転|   
|----|----|----|----|----|   
|BaseColor(RGB) |イメージ/拡散反射|通常|1.0|—|   
|Normal(RGB) |イメージ/法線|通常|1.0|—|   
|Emissive(RGB) |イメージ/発光|通常|1.0|—|   
|Roughness MetallicのMetallic(B)|イメージ/拡散反射|乗算|Metallic Factorの値 (※ ver.0.1.0.9 仕様変更)|o|   
|Roughness MetallicのMetallic(B)|イメージ/反射|通常|1.0|—|   
|Base Color(RGB) |イメージ/反射|乗算|1.0|-|   
|Roughness MetallicのRoughness(G)|イメージ/荒さ|通常|1.0|o|   
|Roughness MetallicのOcclusion(R)|イメージ/拡散反射|乗算|1.0|—|   

(R)(G)(B)は、テクスチャイメージのRGB要素のどれを参照するかを表します。   
glTFでは、Roughness Metallicモデルの場合は「Roughness(G) Metallic(B)」がパックされている、もしくは、   
「Occlusion(R) Roughness(G) Metallic(B)」がパックされて1枚のイメージになっています。   
別途Occlusionイメージがある場合は、「Occlusion (glTF)/拡散反射」の乗算としてマッピングレイヤに追加されます(後述)。   
Ambient Occlusionの効果はマッピングレイヤの拡散反射の乗算として表現されます。    

### Occlusionのマッピングレイヤ (ver.0.1.0.12 対応)

glTFでRoughness MetallicのOcclusion(R)要素を使用せず、単体のOcclusionテクスチャイメージを持つ場合、    
インポート時はglTF Converterプラグインで用意している「Occlusion (glTF)」のレイヤが使用されます。    
これは、「Occlusion (glTF)/拡散反射」として「乗算」合成を割り当てて使用します。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_mapping_layer_occlusion_01.png"/>     
レンダリングした場合、「イメージ/拡散反射」のマッピングレイヤと同じ挙動になります。    

Occlusionテクスチャイメージがマッピングレイヤで指定されている場合は、    
エクスポート時にOcclusionテクスチャとして出力されます。     

制限事項として、Occlusionマッピングレイヤは表面材質のマッピングレイヤで1レイヤのみ指定できます。    
UV1のみを使用、投影は「ラップ（UVマッピング）」になります。    
「適用率」の指定が、glTFのocclusionTexture.strengthになります。    

### 表面材質（マテリアル）の拡散反射値と反射値の関係 (ver.0.1.0.9 対応)

Shade3Dでは、鏡のような反射を表現する場合は拡散反射値を暗くする必要があります。    
glTFではBaseColorを暗くすると鏡のような反射にはならないため、インポート時(glTFからShade3Dへの変換)は以下のように変換しています。    

    const sxsdk::rgb_class whiteCol(1, 1, 1);
    const sxsdk::rgb_class col = glTFのBaseColor Factor;
    const float metallicV  = glTFのMetallic Factor;
    const float metallicV2 = 1.0f - metallicV;
    const sxsdk::rgb_class reflectionCol = col * metallicV + whiteCol * metallicV2;

「拡散反射色」はglTFのBaseColor Factorを採用。    
Roughness Metallicテクスチャがない場合は「拡散反射値」は「1.0 - glTFのMetallic Factor」を採用し、「反射色」は上記で計算したreflectionColを採用。    
Roughness Metallicテクスチャがある場合は「拡散反射値」は1.0、「反射色」は白を採用。    
「反射値」はglTFのMetallic Factorを採用。     

エクスポート時(Shade3DからglTFへの変換)は以下のように変換しています。    

    sxsdk::rgb_class col = (surface->get_diffuse_color()) * (surface->get_diffuse());    
    sxsdk::rgb_class reflectionCol = surface->get_reflection_color();    
    const float reflectionV  = std::max(std::min(1.0f, surface->get_reflection()), 0.0f);    
    const float reflectionV2 = 1.0f - reflectionV;    
    col = col * reflectionV2 + reflectionCol * reflectionV;    

glTFのBaseColor Factorとして上記で計算したcolを採用。    
glTFのMetallic FactorとしてShade3Dの反射値を採用。    

### エクスポート時の表面材質のマッピングレイヤについて (ver.0.1.0.9 対応)

拡散反射の乗算合成としてOcclusionを指定している場合、glTF出力時にはBaseColorにベイクされます。    
別途、glTFのOcclusionテクスチャは出力していません。    
マッピングレイヤの「イメージ/拡散反射」「イメージ/発光」「イメージ/反射」「イメージ/荒さ」を複数指定している場合、glTF出力用に1枚にベイクされます。    
「イメージ/法線」は1枚のみ参照します。    
この場合、以下の指定が使用できます。    

「合成」は、「通常」「加算」「減算」「乗算」「比較（明）」「比較（暗）」を指定できます。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_export_blend_mode.png"/>     
「色反転」、イメージタブの「左右反転」「上下反転」のチェックボックスを指定できます。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_export_mapping_layer_01.png"/>     
「チャンネル合成」の指定は、「イメージ/拡散反射」の場合は「アルファ乗算済み」「アルファ透明」を指定できます。    
「グレイスケール（平均）」「グレイスケール(R)」「グレイスケール(G)」「グレイスケール(B)」「グレイスケール(A)」を指定できます。    

なお、「反復」の指定はver.0.1.0.9でいったん仕様外にしました。正しく動作しません。    

「イメージ/拡散反射」「イメージ/反射」のマッピングレイヤを元に、glTFのBaseColor/Metallicを計算しています。    
「イメージ/荒さ」のマッピングレイヤを元に、glTFのRoughnessを計算しています。    
「イメージ/発光」のマッピングレイヤはglTFのEmissiveのテクスチャとして反映しています。    
「イメージ/法線」のマッピングレイヤはglTFのNormalのテクスチャとして反映しています。    

「頂点カラー/拡散反射」を指定できます(ただし、頂点カラー1のみ使用)。    

## 制限事項

* カメラ情報の入出力には対応していません。  
* Shade3Dでの自由曲面や掃引体/回転体は、エクスポート時にポリゴンメッシュに変換されます。   
* エクスポートされたポリゴンメッシュはすべて三角形分割されます。   
* レンダリングブーリアンには対応していません。  
* 表面材質のマッピングレイヤは、イメージマッピングのみ対応しています。ソリッドテクスチャの出力には対応していません。また、マッピングはUVのみ使用できます。 
* BaseColorのAlpha要素（Opasity）のインポート/エクスポートは、表面材質のマッピングレイヤの「アルファ透明」を参照します (ver.0.1.0.4 追加)。
* スキンは「頂点ブレンド」の割り当てのみに対応しています。
* ジョイントは、ボールジョイント/ボーンジョイントのみに対応しています。
* 表面材質のマッピングレイヤでの反復指定は仕様外にしました(ver.0.1.0.9 仕様変更)。    
* アニメーションは、ボーン+スキンでのモーション指定のみに対応しています(ver.0.1.0.6 追加)。    
* ボーンの変換行列では移動要素のみを指定するようにしてください。回転/せん断/スケールの指定には対応していません。    

## 制限事項 (glTFフォーマット)

* ポリゴンメッシュはすべて三角形に分解されます。   
* ポリゴンメッシュの1頂点にてスキンの影響を受けるバインドされた情報は最大4つまで使用できます。
* 階層構造としては、GLTFのNodeとして出力します。   
この際に、移動/回転/スケール要素で渡すようにしており、Shade3DのGLTF Exporterでは「せん断」情報は渡していません。   
* テクスチャイメージとして使用できるファイルフォーマットは、pngまたはjpegのみです。   
* UV層は0/1番目を使用できます(ver.0.1.0.1 追加)。   
* ポリゴンメッシュの頂点に割り当てる頂点カラーは、頂点カラー番号0のみを乗算合成として使用できます (ver.0.1.0.3 追加)。

## 注意事項    

Shade3Dに3Dモデルをインポートして改変などを行う、またはエクスポートする際は    
オリジナルの著作権/配布条件を注意深く確認するようにしてください。    
個々の3Dモデルの権利については、それぞれの配布元の指示に従うようにしてください。    
glTF Converter for Shade3Dでは個々の3Dモデルの扱いについては責任を負いません。    

## ビルド方法 (開発者向け)

Shade3DプラグインSDK( https://github.com/shadedev/pluginsdk )をダウンロードします。  
Shade3D_GLTFConverter/projects/GLTFConverterフォルダをShade3DのプラグインSDKのprojectsフォルダに複製します。  
開発環境でnugetを使い「Microsoft.glTF.CPP」をインストールします。  
以下のようなフォルダ構成になります。  

```c
  [GLTFConverter]  
    [source]             プラグインのソースコード  
    [win_vs2017]         Win(vs2017)のプロジェクトファイル  
      packages.config    nugetでインストールされたパッケージ一覧.  
      [packages]         nugetでインストールされたパッケージ本体が入る.  
         [Microsoft.glTF.CPP.1.3.55.0]  
         [rapidjson.temprelease.0.0.2.20]  
```

GLTFConverter/win_vs2017/GLTFConverter.sln をVS2017で開き、ビルドします。  

## 使用しているモジュール (開発者向け)

* Microsoft.glTF.CPP ( https://www.nuget.org/packages/Microsoft.glTF.CPP/ )
* rapidjson ( https://github.com/Tencent/rapidjson/ )

rapidjsonは、Microsoft.glTF.CPP内で使用されています。   

## ライセンス  

This software is released under the MIT License, see [LICENSE](./LICENSE).  

## 更新履歴

[2018/07/15] ver.0.1.0.12   
* 表面材質のマッピングレイヤとして、「Occlusion (glTF)」の種類を追加。    
* Import/Export : Occlusionテクスチャイメージの割り当ては、表面材質のマッピングレイヤの「Occlusion (glTF)/拡散反射」を使用するようにした。

[2018/07/13] ver.0.1.0.11   
* Import : マテリアルがdoubleSidedの場合は、マスターサーフェス名に「_doubleSided」を追加
* Export : 微小の面がある場合に、頂点カラーが正しく出力されないことがある問題を修正

[2018/07/05] ver.0.1.0.10   
* Export : UVが存在しない場合は、UVを出力しないように修正
* Import : Node階層で、glTF構造のSkinリストにない終端Nodeはパートのままとした (以前はボーンに変換していた)
* Import : 「イメージのガンマ補正」を有効にした場合に、Emissiveのテクスチャイメージにもガンマ補正をかけるようにした
* Import/Export : イメージ名に全角文字が入っていると不正になる問題を修正

[2018/06/30] ver.0.1.0.9   
* Export : マスターイメージ名で同じものがある場合、イメージが上書きされて意図しないマッピングになる問題を修正
* Export : Shade3DのDiffuse/Reflection/Roughnessテクスチャイメージより、BaseColor/Roughness/Metallicにコンバートする処理を実装
* Export : 複数のテクスチャがある場合に1枚にベイクする処理を実装
* Export : 表面材質のマッピングレイヤの「反復」の対応はいったん仕様外に。
* Export : 面積がゼロの面は出力しないようにした
* Import : マテリアル名で全角の名前を指定していた場合、うまくイメージが読み込めない問題を修正
* Import : テクスチャイメージのガンマ補正をかける場合、BaseColorのみを補正するようにした (それ以外はLinear)
* Import : インポート時のテクスチャイメージのガンマ補正が効かない場合があった問題を修正

[2018/06/23] ver.0.1.0.8   
* Export : エクスポート時のダイアログボックスで、Asset情報として「タイトル」「作成者」「ライセンス」「原型モデルの参照先」を指定できるようにした
* Export/Import : 出力/入力時のglTF/glbのファイルパスで全角のフォルダ名がある場合などに処理に失敗する問題を修正

[2018/06/20] ver.0.1.0.7   
* Export : 表面材質の「イメージ/拡散反射」で合成が「乗算」の場合は、Occlusionとして出力（暫定対応）
* Import : 一部の形状の読み込みで不正になる問題を修正 (glTFのbyteStrideの判別が正しくなかった)

[2018/06/14] ver.0.1.0.6   
* Export/Import : 表面材質のマッピングのテクスチャの反復に暫定対応 (extensionのKHR_texture_transform)。    
* Export/Import : ボーン＋スキンのアニメーション入出力に対応。

[2018/06/07] ver.0.1.0.5   
* Export/Import : 表面材質のマッピングレイヤでの「アルファ透明」は、glTFのalphaModeをBLENDからMASKに変更。    
この場合は、Export時にalphaCutoffを0.9として出力するようにした。
* Export : Export時のマスターサーフェス名の「doubleSided」指定がうまくglTFに反映されていなかった問題を修正。

[2018/06/07] ver.0.1.0.4   
* Export/Import : 表面材質のマッピングレイヤでの「アルファ透明」の対応。
* GLTF → glTF に名称変更。

[2018/06/05] ver.0.1.0.3   
* Export/Import : ポリゴンメッシュの頂点カラーの入出力に対応。頂点カラー0のみを使用します。

[2018/06/01] ver.0.1.0.2   
* Export : スキンを割り当てた場合に、出力結果が不正になる問題を修正。SkinのinverseBindMatricesの出力を行うようにしました。

[2018/05/30] ver.0.1.0.1   
* Export/Import : マテリアルのテクスチャ割り当てにて、UV1も割り当てできるようにした。

[2018/05/27] ver.0.1.0.0   
* 初回版 (Winのみ)
