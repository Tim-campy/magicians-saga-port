# Inventário do APK — M0

Inspeção estática em 2026-10-25. Fonte: APK fornecido na raiz; nenhum download substituto. Resultados reproduzíveis via `tools/inspect_apk.py`; JSON completo em `research/local/apk-inventory.json` (local, ignorado).

## Identidade

| Campo | Valor observado |
|---|---|
| Arquivo | `Magician's+Saga_1.4.0_APKPure.apk` |
| SHA-256 | `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead` |
| Bytes APK | 64.345.815 |
| Package | `net.kairosoft.android.wizardquest` |
| Versão / código | `1.4.0` / `140` |
| SDK mínimo / alvo / compile | 24 / 35 / 35 |
| MAIN + LAUNCHER | `main.Main` |
| Entradas ZIP | 1.399 |
| Soma de bytes descomprimidos | 173.261.265 |
| DEX | `classes.dex`, `classes2.dex`, `classes3.dex` |
| ABIs | `arm64-v8a` (ELF AArch64, 64 bits), `armeabi-v7a` (ELF ARM, 32 bits) |

Identidade Android proveniente do `AndroidManifest.xml` binário, lido pelo parser local. Comparação com apktool/JADX pendente; referências de recursos permanecem numéricas. Assinatura/autenticidade do distribuidor não verificadas. O nome APKPure no arquivo não constitui validação de procedência.

## Bibliotecas

Cada ABI contém as seis bibliotecas abaixo (12 no total), sob `lib/<ABI>/`:

| Biblioteca | `DT_NEEDED` observado nas duas ABIs |
|---|---|
| `libFirebaseCppApp-13_2_0.so` | liblog, libm, libdl, libc |
| `libFirebaseCppMessaging.so` | liblog, libFirebaseCppApp-13_2_0, libm, libdl, libc |
| `libdatastore_shared_counter.so` | libm, libdl, libc |
| `libil2cpp.so` | liblog, libm, libdl, libc |
| `libmain.so` | liblog, libm, libdl, libc |
| `libunity.so` | libmain, libandroid, liblog, libz, libEGL, libmediandk, libm, libdl, libc |

Nomes abreviados na coluna de dependências omitem `.so`. A lista registra links dinâmicos declarados, não bibliotecas carregadas posteriormente via `dlopen` nem comportamento executado.

## Assets e implementação

1.184 entradas em `assets/`, totalizando 41.154.362 bytes descomprimidos. Distribuição: 1.177 em `assets/bin/Data`, uma em `Managed/Metadata`, quatro em `Managed/Resources`, uma em `Data/Resources` e uma diretamente em `assets`.

Extensões: 1.148 sem extensão, 23 `.resource`, cinco `.dat`, três `.json`, uma `.assets`, uma `.config` e três partes `.split0/.split1/.split2`. Cabeçalhos de amostra e lista completa estão no JSON local. Não foram decodificados sprites/fontes/áudio.

Presença conjunta de `libunity.so`, `libil2cpp.so`, metadados IL2CPP e `ScriptingAssemblies.json` confirma a organização Unity/IL2CPP. Este JSON nomeia, entre outras, `Assembly-CSharp.dll`, `KairoLibrary.dll`, Firebase e Tapjoy; isso não significa que DLLs gerenciadas executáveis estejam disponíveis no pacote. A localização exata do gameplay requer análise dos métodos. Veja `02-architecture.md`.

Nenhuma entrada `.obb` encontrada no ZIP. Isso **não** determina se o jogo exige outros downloads ou se funciona integralmente offline.

## Manifesto e serviços

O launcher declara orientação `1`, `hardwareAccelerated=false` e `resizeableActivity=false`. A aplicação declara `debuggable=false`, `extractNativeLibs=true` e `usesCleartextTraffic=true`. São declarações, não observações de execução.

15 permissões `uses-permission`:

```text
android.permission.INTERNET
android.permission.ACCESS_NETWORK_STATE
android.permission.ACCESS_WIFI_STATE
android.permission.POST_NOTIFICATIONS
com.google.android.c2dm.permission.RECEIVE
android.permission.WAKE_LOCK
com.android.vending.BILLING
com.google.android.finsky.permission.BIND_GET_INSTALL_REFERRER_SERVICE
com.google.android.gms.permission.AD_ID
android.permission.ACCESS_ADSERVICES_ATTRIBUTION
android.permission.ACCESS_ADSERVICES_AD_ID
net.kairosoft.android.wizardquest.permission.C2D_MESSAGE
android.permission.ACCESS_ADSERVICES_TOPICS
android.permission.FOREGROUND_SERVICE
net.kairosoft.android.wizardquest.DYNAMIC_RECEIVER_NOT_EXPORTED_PERMISSION
```

Componentes declarados incluem `kairo.android.plugin.notification.FCMMessageService`, Firebase Messaging, Google Mobile Ads, Tapjoy, BillingClient e AndroidX WorkManager. Inventário integral de activities/services/receivers/providers no campo `manifest_tree` do JSON. A obrigatoriedade desses serviços para jogar permanece desconhecida.

## Evidências e limites

REA 6.5.0: `inspect_artifact` parent Evidence `ev_20ad7d1b91933ee9e9939deb6c4a2f6ceac1a5d75bbf616c89ebf2e4997b246d`; manifesto do grafo `agm_98dd77e3d6a49b46a3988e59d8dd706e9d29326e32cac592bb7dd1e46ef3b915`. Bundle local em `research/local/rea-evidence.json`. O grafo conta 1.400 ocorrências incluindo a raiz, portanto não contradiz as 1.399 entradas ZIP contadas pelo Python. Hash REA e PowerShell coincidem.

REA e Python identificaram colisões de nomes: `res/Iw.png` / `res/iW.png` e `res/Lr.xml` / `res/lR.xml`. Extração futura em filesystem sem distinção de caixa precisa preservar essas identidades; não extrair cegamente sobre o mesmo caminho.

CRC de todos os membros foi validado em verificação separada com `ZipFile.testzip()`; o inventariador normal não realiza essa verificação exaustiva. Execuções repetidas geraram JSON idêntico e o hash do APK permaneceu inalterado. A análise não executou o jogo nem decompilou seus métodos.

Esta última descrição refere-se ao inventário inicial. Etapas posteriores mapearam métodos estaticamente, extraíram recursos e implementaram leitores: [extração](12-asset-extraction.md), [mídia Unity](14-unity-media.md), [leitores nativos](15-native-assets.md). O jogo original continua sem execução observada; nenhuma dessas etapas verifica assinatura ou funcionamento offline.
