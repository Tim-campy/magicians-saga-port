# Catálogo IL2CPP — metadados versão 31

Data: 2026-10-25. Inspeção estática, sem executar o APK ou produzir código decompilado. `tools/inspect_il2cpp_metadata.py` lê diretamente o membro ZIP `assets/bin/Data/Managed/Metadata/global-metadata.dat` e escreve o catálogo local em `research/local/il2cpp-metadata-inventory.json`.

## Identidade e método

- APK SHA-256: `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`.
- Metadata SHA-256: `8f79c53ce22e97269f7ac7dd932cd4d80ed83169a5a61f9b9a85fc26d08a0cbd`.
- Tamanho: 7.792.604 bytes. Magic little-endian: `0xFAB11BAF`; versão: `31`.
- Layout de referência: [Il2CppDumper MetadataClass.cs, commit 4741d46](https://github.com/Perfare/Il2CppDumper/blob/4741d46ba9cd6159c5d853eb9d6fc48b4bfa2b1a/Il2CppDumper/Il2Cpp/MetadataClass.cs). Usados apenas os registros aplicáveis à versão 31: header de 256 bytes, type de 88, method de 36, image de 40, assembly de 64, field/parameter de 12. O campo `returnParameterToken` está incluído no registro method v31.

Este é um parser local de escopo limitado, não uma evidência REA. A referência é um schema de ferramenta de terceiros; ainda não foi confrontada com o leitor nativo do próprio jogo. O parser rejeita versões diferentes, intervalos fora do arquivo, seções sobrepostas, registros incompletos, strings inválidas, tokens incompatíveis e relações de ownership inconsistentes.

## Contagens observadas

| Tabela | Registros | Offset inicial decimal | Bytes |
|---|---:|---:|---:|
| Images | 64 | 7.253.524 | 2.560 |
| Assemblies | 64 | 7.256.084 | 4.096 |
| Types | 8.619 | 6.495.052 | 758.472 |
| Methods | 63.273 | 1.924.904 | 2.277.828 |
| Fields | 39.053 | 5.478.308 | 468.636 |
| Parameters | 71.337 | 4.622.264 | 856.044 |

Confiança `confirmed`: contagens e relações estruturais observadas sob o schema v31. As 64 images deste arquivo não equivalem ao número de nomes em `ScriptingAssemblies.json`; os arquivos descrevem conjuntos distintos e não foi estabelecido aqui o motivo exato de cada diferença.

## Candidatos de código do jogo

| Image | Índice | Tipos | Métodos | Intervalo de tipos |
|---|---:|---:|---:|---|
| `KairoLibrary.dll` | 1 | 645 | 6.771 | 1705–2349 |
| `Assembly-CSharp.dll` | 4 | 440 | 4.704 | 4111–4550 |
| `Assembly-CSharp-firstpass.dll` | 15 | 75 | 215 | 7747–7821 |

Os nomes e associações são `confirmed`. A classificação de cada tipo como gameplay, suporte, save ou RNG é `probable` a partir de nomes; nenhum algoritmo foi recuperado.

| Candidato | Type index / offset metadata | Métodos que orientam a próxima pesquisa |
|---|---|---|
| `main.Main` (IL2CPP) | 4166 / `0x68b35c` | `OnCreate` method 34537, `OnUpdate` 34538, `OnDraw` 34539, `OnResume` 34540, `OnSuspend` 34541 |
| `game.Enemy` | 4250 / `0x68d03c` | `Init` 35503, `Update` 35504, `Draw` 35505 |
| `game.QuestRecord` | 4256 / `0x68d24c` | `Serialize` 35548, `Deserialize` 35549 |
| `ecs.BattleSystem` | 4451 / `0x691554` | `Update` 37444, `SearchNearestMonsterInRadius` 37445, `Serialize` 37447 |
| `ecs.EnemySystem` | 4468 / `0x691b2c` | `Update` 37607, `UpdateBattling` 37610, `IsAttackTiming` 37615 |
| `ecs.TownSystem` | 4498 / `0x69257c` | `OnReturnFromQuest` 37993, `Serialize` 38004, `Deserialize` 38005 |
| `data.QuestData` | 4520 / `0x692d0c` | Acessores para nível, distância, limite de tempo e intervalo de inimigos |
| `kairo.unity.io.XGameSaveManager` | 2054 / `0x65dd5c` | `Write` 16871/16872, `Read` 16873/16874, `OnSuspend` 16883 |
| `java.util.JRandom` | 1731 / `0x656e54` | `NextInt` 13183/13184, `NextLong` 13185, `SetSeed` 13186 |
| `kairo.unity.math.Random` | 4325 / `0x68ea04` | `Rand` 36631–36633, `Hits` 36634, `Choose` 36636 |

**Identidades separadas:** `main.Main` no metadata IL2CPP não é automaticamente a classe DEX `main.Main` já investigada. Compartilham nome, mas pertencem a domínios e tabelas distintos. A ligação entre elas ainda não foi demonstrada.

Offsets acima são offsets no arquivo de metadados; method indices são índices de tabela. **Não são endereços de funções em `libil2cpp.so`.** Para chegar aos corpos nativos será necessário mapear `Il2CppCodeRegistration`/`Il2CppMetadataRegistration` e módulos de código. Índices de tipo nativo, assinaturas completas, offsets de campos, chamadas, seeds, fórmulas de combate e formatos de save continuam desconhecidos.

## Reprodução e validação

```powershell
python tools/inspect_il2cpp_metadata.py "Magician's+Saga_1.4.0_APKPure.apk" --output research/local/il2cpp-metadata-inventory.json
```

Validações executadas: duas leituras geraram o mesmo resultado; nove variantes malformadas (truncamento, versão, intervalo de seção, tamanho, owner e string index) foram rejeitadas; SHA-256 do APK permaneceu igual. Todos os métodos de cada tipo apontam de volta ao owner esperado e todas as definições de tipos são cobertas por exatamente uma image. O parser não extrai assets, não grava no APK e não resolve corpos de métodos.

O catálogo JSON contém nomes proprietários e fica local/ignorado pelo Git. Este relatório registra somente os pontos de investigação. Próxima tarefa delimitada: usar o tipo IL2CPP `main.Main` e os três callbacks `OnCreate`/`OnUpdate`/`OnDraw` como sementes para localizar o módulo de código e as funções correspondentes em `libil2cpp.so`.
