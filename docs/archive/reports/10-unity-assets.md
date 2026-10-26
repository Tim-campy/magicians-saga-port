# M0 — Catálogo estático dos recursos Unity

Data: 2026-10-25. APK SHA-256: `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`.
Resultado: há uma cena registrada, `Assets/Scenes/main.unity`, e a cena serializada `level0` referencia diretamente `main.Main` de `Assembly-CSharp`. Foram inventariados recursos e referências sem exportar imagens, áudio, fontes ou conteúdo de tabelas, e sem executar o jogo.

## Método e reprodução

Ferramenta: [`tools/inspect_unity_assets.py`](../tools/inspect_unity_assets.py), Python stdlib + UnityPy **1.25.4**, dependências isoladas em `.tools/unitypy-libs`. O leitor de typetrees C foi desativado; nenhuma rotina de decodificação/exportação de mídia foi chamada. Referências primárias: [UnityPy README](https://github.com/K0lb3/UnityPy/blob/master/README.md), [SerializedFile](https://github.com/K0lb3/UnityPy/blob/master/UnityPy/files/SerializedFile.py), [ObjectReader](https://github.com/K0lb3/UnityPy/blob/master/UnityPy/files/ObjectReader.py), [versão distribuída 1.25.4](https://pypi.org/project/UnityPy/1.25.4/). Os links de código upstream podem mudar; a execução foi fixada à distribuição 1.25.4 e o script rejeita outra versão.

```powershell
python -m pip install --disable-pip-version-check --only-binary=:all: --no-binary=tpk_ar --target .tools/unitypy-libs UnityPy==1.25.4 tpk_ar==0.2.4
python tools/inspect_unity_assets.py "Magician's+Saga_1.4.0_APKPure.apk" --output research/local/unity-assets.json
```

A primeira instalação com somente wheels falhou porque `tpk_ar` não oferece wheel; a instalação acima permitiu apenas o pacote Python `tpk_ar` via sdist e completou localmente. Não houve instalação global. O JSON local contém hashes, offsets absolutos no arquivo lógico, tamanhos, path IDs, nomes, campos selecionados e referências; não deve ser distribuído como conteúdo do jogo. Campos não necessários, inclusive tokens/configuração privada de BuildSettings, não são gravados.

## Integridade do split e cobertura

Os 1.184 membros `assets/` originam 1.182 arquivos lógicos: 1.149 arquivos Unity serialized v22 e 33 outros arquivos. O único grupo split foi concatenado **numericamente** em memória, nunca no APK:

| Membro | Bytes |
| --- | ---: |
| `globalgamemanagers.assets.split0` | 1.048.576 |
| `globalgamemanagers.assets.split1` | 1.048.576 |
| `globalgamemanagers.assets.split2` | 855.944 |
| Total lógico | 2.953.096 |

SHA-256 do resultado: `c00ef7e9b29bfcd7f44a02290edc66bd23f089e91d97359c6a26bd4f01bcd427`. O header v22 declara exatamente 2.953.096 bytes, metadata size 25.694 e data offset 25.744. O script rejeita lacunas/duplicatas de índices, colisão com arquivo não dividido, tamanho declarado inconsistente e objetos fora do intervalo de dados. Os hashes individuais estão em `split_merges` no JSON.

Total: **2.389 objetos**, zero erros de leitura dos campos selecionados. Entre eles:

| Tipo | Quantidade |
| --- | ---: |
| MonoScript | 1.071 |
| Shader / ComputeShader | 1.101 / 6 |
| TextAsset | 38 |
| Texture2D / Sprite | 73 / 11 |
| Font | 5 |
| AudioClip | 23 |
| GameObject / MonoBehaviour | 3 / 7 |

Essas contagens incluem recursos internos Unity, SDKs e recursos de jogo; não equivalem a sprites de gameplay ou funcionalidades distintas. A versão registrada em BuildSettings é `2022.3.62f2`.

## Cena de entrada: referências confirmadas

`globalgamemanagers`, BuildSettings path ID **11**, offset **170720**, tamanho **168**, lista apenas `Assets/Scenes/main.unity`. `level0` possui `GameObject Main` path ID **1**, ativo, com Transform, Camera, AudioListener e três componentes de script. Os ponteiros dos componentes foram resolvidos pela tabela de externals; file ID **1** de `level0` é `globalgamemanagers.assets`.

| GameObject | MonoBehaviour path ID | MonoScript path ID | Classe / assembly |
| --- | ---: | ---: | --- |
| Main | 11 | 883 | `main.Main`, `Assembly-CSharp` |
| Main | 12 | 492 | `system.tapjoy.MyTapjoy`, `Assembly-CSharp` |
| Main | 13 | 881 | `system.notification.FCMReceiver`, `Assembly-CSharp` |
| TapjoyUnity | 14 | 868 | `TapjoyUnity.Internal.TapjoyUnityInit`, `Assembly-CSharp` |
| TapjoyUnity | 15 | 1037 | `TapjoyUnity.Internal.TapjoyComponent`, `Tapjoy.dll` |

MonoScript 883 ocupa offset **127152**, tamanho **64** no arquivo lógico reconstruído. Isso conecta a cena ao tipo IL2CPP identificado em [05-il2cpp-metadata.md](05-il2cpp-metadata.md) e aos endereços de [07-il2cpp-code-map.md](07-il2cpp-code-map.md); não depende apenas da coincidência de nomes. A associação da única cena de build ao `level0` é consistente com o pacote; o fluxo de carregamento em runtime ainda não foi observado. Campos customizados de MonoBehaviour não foram decodificados: apenas cabeçalhos Unity e ponteiros estão cobertos.

TimeManager path ID **8** registra Fixed Timestep ≈ **0,02 s**, Maximum Allowed Timestep ≈ **0,333333 s**, Time Scale **1**. São configurações do motor, não prova de que a lógica do jogo usa um passo de 50 Hz.

## Dados, apresentação e áudio

ResourceManager em `globalgamemanagers` registra **1.161** entradas nomeadas, incluindo `billingmode`, `data/applist.txt` e `data/banner`. O JSON preserva a tabela de nomes e PPtrs sem exportar seus dados.

Dos 38 TextAssets, seis puderam ser lidos como UTF-8: `language_pack_template_ja`, `language_pack_template_en`, `language_pack_format`, `BillingMode`, `snd`, `filelist`. `BillingMode` é JSON com chave `androidStore`; os outros têm formatos textuais ainda sem parser semântico. `filelist` tem 1.108 bytes, `snd` 484. Não se inferiu cobertura completa do conteúdo a partir dessa lista.

Entre os binários opacos estão `enemy` (1.393.315 bytes), `quest` (1.193.989), `xls` (419.822), `chara` (192.940), `connect` (60), `chip` (2.119.126), `hat` (1.487.030) e `effect` (883.220). Muitos começam por `65 b6` e compartilham padrões de header; isso **não identifica** compressão, criptografia ou schema. Nomes indicam candidatos de investigação, não funções comprovadas. Localizações concretas, todos path ID 1:

| TextAsset | Arquivo sob `assets/bin/Data/` |
| --- | --- |
| enemy | `a8601a7ccd1f21f4c909a857fd7809ba` |
| quest | `a8db45a3e5273d649a20c5b7de85cb91` |
| xls | `3078f51cea83c4249a7549cc81302163` |
| connect | `6d3aff8df5d2ea4439b19207595f4ea0` |
| snd | `8bea08ecc76dcf84fa6dff8e7fc9a4a9` |
| filelist | `f83eff5de964c364aa6fcc4a34a7fbe7` |

As cinco fontes contêm dados embutidos: `default-th` 33.872 bytes, `default-en-switch` 2.072.612, `default-hi` 86.736, `default-ja` 1.920.716 e `LegacyRuntime` 50.788. Cobertura de glifos, formato interno e renderização não foram verificados. As texturas incluem ícones/splash/recursos internos; por exemplo `1080x2160` tem dimensões reais **1024×2048**, demonstrando que o nome não deve servir como medida. Os Sprite incluem `108f/b`, `162f/b`, `216f/b`, `324f/b`, `432f/b`; não foi estabelecido que representam sprites de gameplay.

Os **23 AudioClip** possuem referências de stream a arquivos `.resource` locais. Todos os 23 caminhos e intervalos offset+tamanho estão presentes e dentro do APK. Exemplos: `bgm_title` ≈35,085 s, `bgm_main_00` ≈72 s, `bgm_battle_00` ≈36,288 s, `se_button` ≈0,711 s, `jingle_gameover` ≈5,921 s. Duração é campo serializado, não medição de reprodução. Codec numérico, canais, frequência e ranges estão no JSON; não houve decodificação nem teste auditivo.

## Dependências externas e limites

Há componentes Tapjoy/FCM na cena, `assets/UnityServicesProjectConfiguration.json` e strings de domínio como `kairopark.jp` nos metadados IL2CPP. São evidências estáticas de integração/configuração, **não** evidência de requisições, download obrigatório ou endpoint efetivamente usado. URLs XML/XMP como `ns.adobe.com` e `www.w3.org` em dados também não estabelecem tráfego de rede. O scanner guarda apenas hostnames, evitando copiar URLs com parâmetros. Um resultado parcial como hostname `http` pode vir de string truncada; os resultados são pistas sem validação de DNS/rede.

A presença de todos os streams de áudio referenciados é confirmada. A disponibilidade de todo conteúdo de gameplay e o funcionamento offline continuam **unknown**: faltam fluxo de carregamento das tabelas, caminhos de recursos construídos dinamicamente e observação controlada do jogo.

## Validação e próximo passo

Duas execuções completas geraram JSON serializado idêntico ao arquivo salvo, SHA-256 `de272415ae3a354c13be2e6da6de69efd0979a4342250f834f1dbd42edcd8b7d`. Concatenação com entrada fora de ordem produziu ordem numérica correta; cinco casos inválidos (primeiro índice ausente, lacuna, duplicata, arquivo truncado, bytes excedentes) foram rejeitados. Header menor que 48 bytes não é classificado como serialized v22. Foram conferidos os cinco links de script e os 23 ranges de áudio. Nenhuma mídia foi escrita e o APK permanece somente leitura.

Próximo experimento limitado: seguir chamadas de carregamento de `filelist`/`data/xls` no IL2CPP e identificar o decoder dos TextAssets opacos por evidência de código. Só depois definir schema e teste de round trip de um recurso pequeno. O inventário não autoriza substituir tabelas por aproximações nem iniciar o port de gameplay.

## Complemento: seleção do experimento sobre TextAssets `65 b6`

O script mede entropia de Shannon sobre a distribuição de bytes de cada payload, arredondada a seis casas, sem transformar conteúdo. **30 dos 38** TextAssets começam por `65 b6`. Sua entropia varia de **5,143983 a 7,999780 bits/byte**; o tamanho de amostra varia muito, então não é uma classificação de codec ou proteção.

| Nome | Bytes | Entropia bits/byte | Primeiros 16 bytes |
| --- | ---: | ---: | --- |
| connect | 60 | 5,143983 | `65b6489d7a082f1bfdaa4bebc6fb7145` |
| load | 18.762 | 6,763529 | `65b6480c7a0867b6fdaa4beec6fb7145` |
| xls | 419.822 | 7,614163 | `65b64ed17a0e4e7dfdaa4bc5c6fb7154` |
| enemy | 1.393.315 | 6,583559 | `65b6401d7a1d16fcfdaa4b8ac6fb7145` |
| quest | 1.193.989 | 7,027878 | `65b658697a1a082efdaa4b73c6fb7145` |

Não foi encontrada uma prova do significado desses campos neste experimento. A variação de entropia e o prefixo recorrente não autorizam chamar o formato de criptografado ou comprimido.

O catálogo IL2CPP e o mapa nativo já validados fornecem candidatos concretos de **KairoLibrary.dll**, todos em `kairo.unity.io.AssetReader`:

| Método | Índice metadata | VA ELF ARM64 |
| --- | ---: | --- |
| `GetInputStream` | 16888 | `0x1c8f940` |
| `_getInputStream` | 16889 | `0x1c8fd28` |
| `GetData` | 16894 | `0x1c8fa48` |
| `_getData` | 16892 | `0x1c909b0` |
| `_getTextAsset` | 16893 | `0x1c90b14` |
| `SetSecurity` | 16898 | `0x1c91234` |

Existem também `kairo.unity.util.Encrypter.Decode` nos índices **13788**, **13789**, **13791**, VAs `0x1bf3cdc`, `0x1bf3d3c`, `0x1bf3e14`. A existência dessa classe **não prova** que ela é chamada para os TextAssets `65 b6`; pode servir a outro subsistema. Nenhum algoritmo, chave ou decoder foi implementado/inferido a partir desses nomes.

Experimento recomendado: inspecionar primeiro `_getTextAsset` e `GetData` com fronteiras de função verificadas; estabelecer a cadeia entre `TextAsset.bytes`, eventuais transformações e o chamador que abre `data/xls`/`data/connect`. Só então verificar se o prefixo `65 b6` é consumido por essa cadeia e quais bytes são comprimentos ou flags. `connect` é candidato pequeno (60 bytes) para uma futura prova de layout, mas seu nome não comprova conexão de rede. Até haver essa ligação, o formato permanece **unknown**.
