# Leitura de recursos: caminho estático e decisão pendente

**Atualização:** o usuário autorizou prosseguir com a investigação de chave e extração. A pausa abaixo é histórica; resultados e extrator validado em `12-asset-extraction.md`.

Data: 2026-10-25. Mesma identidade de APK/ELF de `07-il2cpp-code-map.md`. Nenhum recurso foi decodificado nesta investigação e nenhuma chave foi recuperada.

## Evidência

Seleções FDE em `build/research/asset-reader.tsv` e `codec-methods.tsv`; assembly local em diretórios homônimos. Análise Ghidra direta no projeto `wizard_il2cpp_fde`, sem autoanalysis ampla. Uma tentativa concorrente foi rejeitada pelo lock do projeto enquanto a tarefa anterior concluía; repetição sequencial passou, sem remover locks.

| Método | Metadata index | VA ELF | Bytes FDE |
|---|---:|---|---:|
| AssetReader._getInputStream | 16889 | 0x1c8fd28 | 364 |
| AssetReader._getData | 16892 | 0x1c909b0 | 348 |
| AssetReader._getTextAsset | 16893 | 0x1c90b14 | 364 |
| AssetReader.GetData | 16894 | 0x1c8fa48 | 736 |
| Encrypter.Decode | 13788 | 0x1bf3cdc | 96 |
| Encrypter.Decode (sobrecarga) | 13789 | 0x1bf3d3c | 100 |
| Encrypter.Encode (sobrecarga) | 13787 | 0x1bf3c68 | 116 |

Os três leitores privados constroem `UIMethod` e chamam `IApplication.RunOnUiThread` (`0x1b7b428`). O corpo delegado ainda não foi resolvido; não afirmar que a leitura final já está mapeada.

`GetData` chama `_init` (`0x1c8f75c`), despacha por RunOnUiThread, testa nome de recurso com `String.Equals`/`String.EndsWith` e flags estáticas. Um caminho chega ao BL em **0x1c8fcf8** para **Encrypter.Decode 0x1bf3cdc**. As strings e flags não foram resolvidas: não extrapolar esse caminho a todos os TextAssets de prefixo `65b6`.

Decode lê o primeiro ponteiro do armazenamento estático de sua classe e transfere para a sobrecarga; essa sobrecarga transfere para Encode. No corpo de Encode, os bytes do primeiro array sofrem XOR com bytes do segundo array, indexados periodicamente por resto da divisão pelo comprimento do segundo. A transformação ocorre no próprio array. É evidência de uma rotina reversível com chave, não identificação do formato final dos assets ou da origem dessa chave.

Não foram investigados valores da chave, sua inicialização, alteração em runtime, relação com controles de licença nem dados resultantes. A mera presença da rotina não prova um mecanismo de licenciamento.

## Pausa da investigação de extração

A skill local `universal-decompiler:asset-extraction`, em `C:/Users/l_abn/.codex/plugins/cache/universal-decompiler/universal-decompiler/0.1.0/skills/asset-extraction/SKILL.md`, contém a instrução explícita: **“Encrypted assets (a key needed to read them) are a protection: stop and tell the user.”**

A investigação de extração foi interrompida após identificar o uso de chave e antes de recuperá-la ou decodificar dados. Essa é uma restrição do workflow da skill; não uma conclusão jurídica nem prova de DRM. Próximo caminho deve ser definido com o usuário. Alternativas sem essa etapa: continuar análise estática dos demais subsistemas ou comparar o jogo original em dispositivo Android autorizado.

## Validação adjacente

O inventariador `inspect_arm64_calls.py` produziu saída idêntica em repetição; rejeitou identidade ELF divergente antes de criar saída. A comparação com o assembly e suas lacunas está documentada em `09-native-callbacks.md`. Não há teste runtime ou build jogável.
