# Ordem do registro e localização do bloco do mundo

Data: 2026-10-25. Mesmo APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead` e ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. VAs ELF; exports Ghidra adicionam0x100000. Investigação estática, sem arquivo de save real.

## Dictionary.Values na escrita

O GetEnumerator chamado por EntityManager.Serialize em45 é0x19e4b08. Lê o dicionário armazenado no objeto Values+0x10 e encaminha ao construtor0x15918c4. Este guarda o dicionário, índice0, current=null e cópia da versão dictionary+0x2c.

MoveNext0x15918ec compara essa versão com a atual antes de avançar. Divergência chama o helper excepcional0x1e825f4; o método não implementa snapshot. A varredura usa índice crescente até dictionary+0x20, lendo entradas do array dictionary+0x18, com stride24 bytes. Entrada com hash negativo é pulada; entrada ativa fornece seu valor em entry+0x10, correspondente à entidade. Ao terminar, current=null e índice=limite+1. Dispose0x15918e8 é RET.

Isso confirma que a ordem não é uma ordenação por ID. As duas passagens de45 começam de índice0 e visitam as entradas ativas na mesma ordem **se o registro permanecer estável entre elas**. Cada enumerador captura uma nova versão; não há verificação global que compare a versão da primeira passagem com a da segunda. Efeitos dos hooks sobre o registro continuam relevantes. Não foi inferida política de reutilização de entradas de Dictionary a partir da implementação diferente de HashSet.

## Chamadas diretas ao mundo

Varredura das instruções alinhadas BL/B nos segmentos executáveis, com atribuição do caller somente quando o endereço cai dentro de FDE associado a método conhecido, encontrou:

| Operação | Site | Caller |
|---|---|---|
| World.Serialize37013,0x11358f8 | 0x1107434 | AppData.PackSave34314 |
| World.Deserialize37014,0x1135938 | 0x11086b4 | AppData.LoadGame34324 |

Resultado local `build/research/world-save-direct-callers.json` conserva hash e escopo. Essa busca não cobre chamadas indiretas/reflexivas nem prova que cada ramo seja executado em runtime.

World.Serialize37013 (64 bytes) chama EntityManager.Serialize através de World+0x20 e depois SystemManager.Serialize através de World+0x28, com o mesmo stream. Essa ordem corresponde a World.Deserialize de36. O bloco de entidades isolado não equivale ao bloco completo do mundo.

## Caminho de LoadGame até o bloco binário

LoadGame34324 (`0x11083f8`,2056 bytes) inicia chamando AppData.LoadSystem34319 (`0x1107e8c`). Depois chama RecordStore.OpenRecordStore17003 (`0x1c98208`) com0 e ReadRecord17006 (`0x1c98344`) com0 e o argumento de slot acrescido de1. Os detalhes de tratamento de RecordStore permanecem nos alvos de20.

O método constrói SecureProperty14304 (`0x1c07a98`), guarda o objeto em AppData+0x198 e chama SecureProperty.Load14315 (`0x1c08470`) com os bytes obtidos. Há ainda AppData.DeserializeData34317 (`0x11077b4`) antes de carregar o mundo. Os efeitos desses callees ainda precisam ser seguidos; não declarar que o registro existente já foi limpo.

Em0x1108608–0x110864c, lê a referência em SecureProperty+0x10, acessa o elemento **13** desse array (offset0x88 contando o cabeçalho), e fornece o elemento ao construtor **ByteArrayInputStream13499,0x1be8544**. Esse stream é passado a World.Deserialize em0x11086b4. Portanto o consumidor concreto desse bloco usa ByteArrayInputStream, não um stream .NET escolhido por convenção.

PackSave34314 (`0x1107304`,1196 bytes), no caminho cujo seletor é13, chama World.Serialize em0x1107434 com seu stream de saída. A correspondência do seletor13 com o elemento13 do carregador sustenta o papel desse bloco do mundo. A montagem completa de SecureProperty, a origem de todos os seletores e os demais blocos ainda não foram recuperados; não inferir que13 seja offset de arquivo, número do record ou versão do save.

O wrapper ByteArrayInputStream é próximo alvo para verificar se uma leitura incompleta retorna negativo, zero ou lança erro. A observação de42 sobre o loop genérico de StreamUtil continua válida, mas não demonstra travamento nesse caminho concreto sem examinar o stream que ele realmente recebe.

## Validação e próximos passos

Novos exports `build/research/save-dictionary-order.tsv` e `save-callers.tsv`: getters/enumerador, construtor, World.Serialize, PackSave e LoadGame com cobertura sintática integral. Os dois callers maiores foram examinados nos trechos necessários ao caminho acima; cobertura integral de instruções não significa recuperação de todos os seus comportamentos.

Nenhum C++ alterado, nenhum build repetido e nenhum save aberto. Próximo: ByteArrayInputStream.Read e LoadSystem/DeserializeData para fechar EOF e preparação do mundo; depois SecureProperty e RecordStore para formato externo e integridade. Goal ativo, análise estática ainda útil.
