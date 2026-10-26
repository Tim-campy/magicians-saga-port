# Extração comprovada dos contêineres de recursos

Data: 2026-10-25. O usuário autorizou explicitamente prosseguir com investigação de chave e extração, substituindo a restrição de workflow registrada em `11-resource-reader.md`. APK original permanece intacto. Nenhuma alteração em billing, licença, rede ou executável foi realizada.

## Resultado e reprodução

`tools/extract_assets.py` lê a cópia local **1.4.0 com SHA-256 d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead**. A chave não é distribuída no script: vem do metadata dessa cópia. Saída em `data/extracted`, excluída pela regra `data/` do `.gitignore`.

```powershell
uv run tools/extract_assets.py "Magician's+Saga_1.4.0_APKPure.apk" --out data/extracted
uv run tools/extract_assets.py --check --out data/extracted
```

O script tem dependências PEP 723 fixadas. Nesta sessão foi executado com `python`, usando UnityPy 1.25.4 já instalado em `.tools/unitypy-libs`, e depois com `uv run` (extração e verificação passaram). Os mesmos argumentos são aceitos por `python tools/extract_assets.py` nesse ambiente.

- 38 TextAssets extraídos: 30 contêineres decodificados, oito payloads preservados.
- **1.963 membros internos**: 973 PNG, 303 SEB, 286 OPTINFO, 286 OPT, 63 INF, 47 TXT, três CSV, um BIN e um MAB.
- Manifesto registra origem, path ID, nome original, transformação, tamanho e hashes antes/depois. Nomes de saída recebem índices para evitar colisões Windows; nomes originais continuam no manifesto.
- Não substitui arquivos existentes diferentes; rejeita APK de hash diferente, limites inválidos e formatos de compressão ainda não suportados.

## Cadeia de evidência da chave

1. `AssetReader.GetData` chega a `Encrypter.Decode`, conforme `11-resource-reader.md`.
2. `.cctor` de Encrypter (`0x1bf3e78`) começa com um array de um byte; portanto não bastava extrair a inicialização padrão.
3. Varredura de BL/B encontra SetEncryptionKey em `AppData.Init` (`0x1100298`), call site **0x11006e4**. Outros call sites existem em ApplicationManager; não presumir chave global imutável.
4. Em Init, `0x110062c` passa 11 para alocação de array; `0x1100640` chama `RuntimeHelpers.InitializeArray`; o loop até `0x11006c0` expande os onze inteiros em **44 bytes little-endian** e passa o array ao setter.
5. Referência carregada pelo código em VA ELF `0x272fa50` tem relocation para `0x27ea1c0`, cujo valor inicial é `0x800000f5`. O schema v31 decodifica uso 4 / fieldRef 122. [Il2CppDumper Metadata.cs, commit fixado](https://github.com/Perfare/Il2CppDumper/blob/4741d46ba9cd6159c5d853eb9d6fc48b4bfa2b1a/Il2CppDumper/Il2Cpp/Metadata.cs) fornece a fórmula de índice para versões ≥27.
6. fieldRef 122 contém type index 8809 e índice relativo de campo 122. A resolução completa de tipos nativos ainda não está implementada. Entre candidatos `<PrivateImplementationDetails>`, campo 24349 fornece initializer de 44 bytes cujo SHA-256 coincide exatamente com seu nome gerado; os outros dois candidatos não coincidem para esse tamanho. Offset no metadata: **4557992**, comprimento **44**, default-data index **188696**. Essa identificação é corroborada pelos 30 contêineres decodificados e validação integral dos PNGs.
7. Setter `0x1bf36f0` armazena o array; Decode `0x1bf3cdc` → sobrecarga `0x1bf3d3c` → Encode `0x1bf3c68`. Este último realiza XOR periódico in-place. Nenhum serviço ou segredo de conta é necessário para esses arquivos.

O extrator é deliberadamente específico a esta versão, com hash obrigatório. Não afirma que o mesmo offset/chave se aplique a outros APKs.

## Contêiner JarInflater

Leitor original: `kairo.unity.io.JarInflater._load`, índice 13963, VA **0x1bfb5d0**. Assembly/pseudocódigo local em `build/research/jar-load`. A sequência de leituras e arrays sustenta o layout abaixo; a reconstrução exata de todos os arquivos fornece validação independente.

Todos os inteiros do contêiner são unsigned big-endian nesta interpretação dos arquivos observados:

| Campo | Formato |
|---|---|
| Primeiro tamanho A | u32; índice ocupa A bytes após o primeiro u32 |
| Tamanho dos dados B | u32 |
| Quantidade N | u32 |
| N nomes | u32 comprimento + bytes UTF-8 |
| N offsets | u32 relativo ao início dos dados |
| N tamanhos | u32 |
| N flags | u8; todos zero nesta versão observada |
| Dados | cada membro tem u32 comprimento + bytes |

Fim do índice = `A+4`; tamanho total = `A+B+4`. Os membros observados são contíguos, sem lacunas; cada comprimento interno coincide com a tabela. O script rejeita flags não zero em vez de inventar um codec.

## Validação executada

- Os 30 payloads passaram por XOR inverso e comparação exata com a entrada.
- Os 30 contêineres foram parseados, reserializados e comparados byte a byte.
- **973 PNGs** passaram por `Pillow.Image.verify()` e decodificação completa com `load()`; imagem `title.png` também foi inspecionada visualmente e apresenta o título do jogo.
- Inventário de dimensões: nenhuma dessas 973 imagens excede 1.024 pixels em qualquer eixo; expansão simultânea hipotética de todas para RGBA8 soma 53.709.576 bytes. Isso não mede consumo real, VRAM ou viabilidade 3DS; relatório local `research/local/extracted-image-budgets.json`.
- `--check` verificou hashes e tamanhos dos 38 TextAssets e 1.963 membros.
- Segunda extração no mesmo destino passou sem alterar saídas diferentes; manifesto determinístico.
- Seis testes sintéticos em `tests/test_extract_assets.py` passaram: reconstrução, truncamento/dados extras, flag desconhecida, comprimento divergente, XOR/chave vazia e saída fora do diretório.

```powershell
python -m unittest discover -s tests -p "test_extract_assets.py"
```

## Limites e próxima etapa

Extração de TextAssets e contêineres comprovada; **M1 ainda parcial**. O modo padrão cobre esses contêineres; a opção posterior `--unity-media` também exporta objetos Texture2D/Font/AudioClip, conforme [14](14-unity-media.md). SEB/OPT/OPTINFO/MAB permanecem dados originais sem interpretação completa; disposição de sprites, animações, fontes e reprodução de áudio precisam de evidência. Não há renderer nativo nem comparação runtime. Próximo alvo: relacionar INF/SEB aos PNGs e aos leitores originais, sem aproximar coordenadas ou tempos.
