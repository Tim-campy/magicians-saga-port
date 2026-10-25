# Arquitetura — M0 parcial

Data: 2026-10-25. `confirmed` significa observação estática validada sob o schema indicado; `probable`, inferência; `unknown`, ainda não estabelecido. Nenhum comportamento runtime foi observado. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`.

## Estrutura evidenciada

Unity/IL2CPP está confirmado por bibliotecas, metadata v31 e recursos. O catálogo identifica 64 images, 8.619 tipos e 63.273 métodos. O mapa associa tokens e relocations a 56.955 VAs executáveis distintas, com 6.318 slots nulos não resolvidos. KairoLibrary.dll e Assembly-CSharp.dll contêm candidatos ao framework e jogo; isso não torna todo DEX um mero launcher. Fontes: [05](05-il2cpp-metadata.md) e [07](07-il2cpp-code-map.md).

O caminho Android é DEX `main.Main.onCreate` → UnityPlayerActivity → UnityPlayer → carregamento nativo → thread UnityMain/handler T0 → nativeRender. JADX está disponível; fallback de carregamento, foco e reagendamento foram conferidos em smali ([04](04-android-startup.md), [08](08-smali-validation.md)). O registro JNI NativeLoader.load/unload e as bibliotecas carregadas estão em [06](06-native-bootstrap.md). O caminho interno de Unity até a ativação da lógica ainda não está integralmente mapeado.

BuildSettings registra a cena `Assets/Scenes/main.unity`. Em `level0`, GameObject Main tem MonoBehaviour 11, que referencia MonoScript 883 de globalgamemanagers.assets: **main.Main de Assembly-CSharp**. Esse tipo IL2CPP é distinto do DEX homônimo. A cena também referencia MyTapjoy, FCMReceiver e componentes TapjoyUnity ([10](10-unity-assets.md)). A ordem runtime de ativação permanece desconhecida.

Os callbacks IL2CPP main.Main.OnCreate, OnUpdate e OnDraw estão em **0x117c78c**, **0x117cdc8** e **0x117db20**, respectivamente: VAs ELF ARM64, não offsets nem endereços runtime. Intervalos FDE e chamadas conferidas estão em [09](09-native-callbacks.md). FDE fornece cobertura de unwind, não prova absoluta do corpo lógico.

Globalgamemanagers registra Unity 2022.3.62f2; unity default resources registra 2022.3.54f1. Preservar essa diferença sem atribuir a todos os componentes uma única versão de geração.

## Mapa de subsistemas

| Subsistema | Evidência e confiança | Lacuna |
| --- | --- | --- |
| Boot/cenas | **confirmed:** manifesto, DEX/smali, JNI e referência de cena para main.Main (04, 06, 08, 10) | Ativação completa, primeiro formulário e captura de boot |
| Loop/timing | **confirmed:** OnUpdate chama FormManagerBase.Execute e referencia vSyncCount, GetTargetFps e targetFrameRate (09). TimeManager configura passo ≈0,02 s (10) | Simulação versus apresentação; valores iniciais e taxa efetiva. Não assumir gameplay fixo a 50 Hz |
| Renderização | **confirmed:** SetViewport armazena argumentos 240/280 em campos; OnDraw chama GetInstance e despacha indiretamente (09) | Slot virtual, escala, composição e resolução física |
| Input | **confirmed:** wrappers de toque alcançam nativeInjectEvent; OnUpdate chama Canvas.CheckKeyState (04, 09) | Coordenadas, gestos, estados e comparação de controle |
| Áudio | **confirmed:** 23 AudioClip com streams locais válidos; SetJingle alcança SoundPlayer.SetVolume (09, 10) | Playback, unidades de volume, mixer e sincronização |
| Recursos | **confirmed:** 1.149 arquivos Unity, 2.389 objetos; split validado; 38 TextAssets (10). Extração autorizada produziu 1.963 membros e 973 PNGs validados | Schemas de animação/tabelas, apresentação fiel e cobertura de conteúdo |
| UI/combate/IA | **confirmed:** tipos e endereços, chamadas ao framework de formulários. **probable:** responsabilidades sugeridas por ecs.BattleSystem/EnemySystem (05, 07, 09) | Regras, estado e cenários comparáveis |
| Progressão/economia | **confirmed:** candidatos ecs.TownSystem/data.QuestData, recursos quest/equipment/shop | Fórmulas, transações, schema e cobertura |
| Persistência/RNG | **confirmed:** chamadas RecordStore.ReadRecord e SaveRecorder.SaveSystem; candidatos XGameSaveManager e RNG (05, 09) | Caminhos, formatos, condições, algoritmo e seeds |
| Serviços/completude | **confirmed:** Tapjoy/FCM na cena, billing/rede no manifesto | Obrigatoriedade, downloads e offline; presença local não comprova completude |

## Recursos e limites

AssetReader.GetData chama Encrypter.Decode em um caminho condicionado ([11](11-resource-reader.md)). Após autorização explícita do usuário, a extração foi retomada: 30 TextAssets transformados e oito preservados, com verificação de contêineres e hashes. Resultados permanecem locais em `data/extracted`; `tools/extract_assets.py` é o caminho reproduzível. A extração não comprova semântica das tabelas nem fidelidade de animação. Os próximos formatos são `.seb` e `.inf`.

Três tentativas Ghidra não produziram pseudocódigo confiável de OnUpdate: ele absorve trechos de callees. Usar as observações de assembly delimitado do relatório 09, preservando divergências de cobertura. A contagem de classes JADX direta difere da contagem REA; o relatório 08 registra a discrepância. Nenhuma dessas limitações foi tratada como sucesso funcional.

## Direção do port

Sem implementação de gameplay: `src/core` concentrará regras; `src/assets`, leitores; `src/platform`, vídeo, áudio, input, tempo, arquivos e eventual rede. O primeiro leitor usa C++17/CMake, sem dependências de plataforma; seu diferencial de contêineres passou em Windows (`15`). O visualizador técnico opcional usa **SDL3** exclusivamente em `src/platform/desktop`, com stb_image para PNG (`19`): escolha local ao protótipo, seguindo a skill de reconstrução e sem acoplar o core à biblioteca. Não define ainda o backend final nem demonstra viabilidade 3DS. O produto final não dependerá de runtime Android. Timing, RNG e serialização exigem comparação com o original.

Viabilidade Windows/Linux **não demonstrada**: regras, formatos e dependências ainda precisam de confirmação. Viabilidade 3DS **não demonstrada**: faltam medidas de RAM, VRAM, CPU, texturas, controles e telas. M0 permanece parcial.
