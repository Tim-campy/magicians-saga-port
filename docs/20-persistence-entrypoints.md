# Persistência: pontos de entrada, sem formato recuperado

Data: 2026-10-25. Análise estática do ELF ARM64 identificado em `07-il2cpp-code-map.md`. Nenhum save do usuário foi lido, escrito ou alterado; nenhum formato de save foi implementado.

## Evidência delimitada

Assembly local em `build/research/resource-methods`, a partir de limites FDE. Os destinos abaixo são correspondências do mapa de tokens/relocations, não execução observada.

| Origem | Endereço/call site ELF | Destino observado |
|---|---|---|
| RecordStore..ctor, índice 16987 | 0x1c96584 | Storage.Open, 0x1c997b4 |
| mesmo construtor | 0x1c96660 | Storage.Read, 0x1c9b8b0 |
| RecordStore.ReadRecord, índice 17006 | 0x1c9834c / 0x1c98358 | OpenRecordStore e sobrecarga ReadRecord |
| RecordStore.ReadRecord, índice 16998 | 0x1c97d60 | RecordStore.GetFileName, 0x1c97138 |
| mesma sobrecarga | 0x1c97d98 | KairoPlugin.GetPreference, 0x1c27f10 |
| mesma sobrecarga | 0x1c97db0 | Storage.Read, 0x1c99dd0 |
| mesma sobrecarga | 0x1c97e0c / 0x1c97e24 | Duas sobrecargas Encrypter.Decode |
| mesma sobrecarga | 0x1c97e34 / 0x1c97e80 | EncodeUtil.ByteAryToLong e CRC64.GetValue |
| mesma sobrecarga | 0x1c97ec0 | IllegalFileException..ctor |
| SaveRecorder.SaveSystem, índice 18925 | 0x1bac2a4 / 0x1bac2c8 | ThreadStart..ctor e IApplication.RunOnUiThread |

O leitor contém caminhos alternativos de Storage.Read; a tabela não afirma que todos os destinos sejam executados em sequência. A presença de CRC64 e de exceção indica uma verificação a investigar, mas não estabelece polinômio, posição do checksum, campos cobertos ou condição de rejeição. Não remover essa verificação no futuro port.

SaveSystem delega trabalho, portanto ler seu wrapper não recupera o corpo que serializa o save. Tampouco o nome RecordStore prova um formato RMS/J2ME convencional: o arquivo deste APK é a referência.

## Próximos passos

1. Resolver GetFileName, Storage.Open/Read e o delegate de SaveSystem para localizar caminhos e limites de plataforma.
2. Mapear a ordem exata de decodificação e integridade e identificar o serializer efetivamente usado pelo jogo.
3. Somente após isso, obter uma cópia de save em ambiente autorizado para comparação e round trip, preservando o original. Não criar schema aproximado nem tentar satisfazer checksums por tentativas.
