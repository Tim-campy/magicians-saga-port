# Inicialização do save de sistema e carga de BaseData

Data: 2026-10-25. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`; ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Investigação estática; VAs ELF, exports Ghidra com base adicional0x100000.

## InitSystemSaveData é a inicialização de LoadSystem

O destino0x1101a10 identificado em47 resolve **AppData.InitSystemSaveData34261**,568 bytes. O método obtém cinco referências estáticas de AppData em+0x2b0,+0x320,+0x328,+0x330,+0x338 e as fornece ao construtor SecureProperty14304. Armazena o objeto resultante em AppData+0x190.

Depois há dois caminhos condicionais que escrevem objetos em posições do array do SecureProperty+0x20, usando índices obtidos de estáticos AppData+0x2ec e+0x2c8. O primeiro depende do retorno de0x1b1b4f4; o segundo depende de flag estático+0xc1 e comparação do campo AppData+0x1a0. Identidades funcionais dessas opções não foram presumidas a partir dos offsets. Ambos obtêm o valor por chamada0x1c068a8 com argumento1 ou0 respectivamente.

O corpo não cria World nem chama diretamente DestroyAllEntities/RemoveAllSystems. Isso identifica o que LoadSystem prepara diretamente: o objeto do save de sistema. Não prova ausência de efeitos indiretos em construtores/callees nem autoriza retirar a pendência de preparação ECS.

## DeserializeData atua sobre BaseData[]

AppData.DeserializeData34317 (`0x11077b4`,956 bytes), exportado em47, recebe uma tabela de descritores, SecureProperty e um booleano. Em cada descritor lê o primeiro elemento e verifica/converte seu tipo para um array específico. Essa identidade foi agora resolvida:

GOT0x272fb08→0x27c4030→referência0x20003ebf→native type8031. O tipo nativo é SZARRAY em0x26b80b0; seu elemento0x281b1c8 tem kindCLASS e definição4503, **data.BaseData**. Assim o alvo da desserialização é BaseData[], não uma entidade ECS inferida pelo nome genérico do método.

O segundo elemento do descritor é convertido em texto e passado a JInteger.ParseInt13380. O índice resultante seleciona um array de bytes no campo SecureProperty+0x10. Para alvo não nulo, cria ByteArrayInputStream desse bloco e segue:

1. Lê ReadShort e aplica extensão de sinal para a quantidade de registros.
2. Para cada registro, lê índice16 com sinal e tamanho32.
3. Se índice>=comprimento do array, ou a posição estiver null, chama slot13 do stream com o tamanho convertido para64 bits. O significado exato desse slot deve ser confirmado antes de portar a operação de pulo.
4. Caso haja objeto no índice, booleano false chama slot10 do BaseData e depois slot11; true chama apenas slot13 do BaseData.
5. Ao terminar a seção, chama slot6 do stream e segue para o descritor seguinte.

A vtable de BaseData4503 resolve slot10→**Deserialize38054**, slot11→**Deserialized38055**, slot13→**DeserializeShare38057**. São alvos virtuais, portanto objetos derivados podem sobrescrevê-los; os corpos por tipo ainda precisam de investigação. O caminho LoadGame observado em46 passa false e usa Deserialize seguido de Deserialized.

Índice negativo não é ignorado como índice grande: passa pela comparação signed com comprimento e chega à checagem unsigned, que segue para falha de bounds. Quantidade de registros<1 pula o loop. No caminho com objeto presente, o tamanho32 lido não é usado para limitar o stream entregue a Deserialize; a rotina concreta precisa consumir seu próprio payload. Não envolver automaticamente cada registro em uma substream limitada sem evidência adicional.

## Implicação para a preparação do mundo

LoadGame carrega BaseData antes de entregar o bloco13 a World.Deserialize (46). Isso estabelece uma ordem entre dados e ECS, mas ainda não identifica a limpeza ou a criação necessária antes de registrar entidades. A ausência de limpeza explícita em InitSystemSaveData não exclui efeitos de hooks BaseData nem preparação pelo caller de LoadGame. A investigação não foi encerrada por falta de execução dinâmica; há alvos estáticos concretos.

## Validação e próximos passos

Novo export `build/research/save-system-init.tsv`,568 bytes com cobertura sintática integral, e releitura do export de DeserializeData em `save-input-preparation/`. Confrontados referências de tipo SZARRAY, definição BaseData e slots de vtable. Nenhum C++ alterado, nenhum save aberto e nenhum build repetido.

Próximos alvos: origem da tabela de descritores entregue por LoadGame, overrides BaseData relevantes e callers de LoadGame; identificar onde World é limpo antes de integrar carregamento ECS. Confirmar também os slots do stream usados para pular/finalizar seções. Goal ativo.
