# Destruição e carregamento de entidades em duas passagens

Data: 2026-10-25. Mesmo APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead` e ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Análise estática; nenhum save foi aberto nem APK executado. VAs ELF; exports Ghidra adicionam0x100000.

## Ordem da destruição

EntityManager.DestroyEntity37084 (`0x1137950`) exige entidade não nula e segue:

1. Chama slot6 da entidade, BaseEntity.RemoveAll37062 no tipo concreto examinado.
2. Chama Entity.DisposeEvents36911 (`0x1131adc`), que zera os três delegates entity+0x10/+0x18/+0x20.
3. Se o booleano recebido estiver ligado, remove do dicionário manager+0x18 usando entity+0x28 como chave. Se desligado, mantém o registro.

A remoção de componentes ainda pode emitir notificações e alterar grupos, porque DisposeEvents vem depois. Não desligar callbacks antes de RemoveAll por conveniência. Falha na remoção interrompe o caminho antes do desligamento/remoção do registro; não há rollback ou finally nesses152 bytes.

DestroyAllEntities37085 (`0x11379e8`) obtém Dictionary.Values, chama DestroyEntities37086 (`0x1137a6c`) com exceção=null e booleano=false, e só depois chama Dictionary.Clear. As identidades foram resolvidas por referências genéricas: GOT0x2730e48→0xc001a11b→spec(12523,399,-1)=get_Values; GOT0x2730e40→0xc001a111→(12532,399,-1)=Clear. DestroyEntity usa GOT0x2730e38→0xc001a115→(12545,399,-1)=Remove.

DestroyEntities enumera a entrada, pula a entidade igual ao argumento de exceção e chama Entity.Destroy36910 (`0x1131aa0`) nas demais, passando o booleano. O caminho normal de DestroyAll evita a remoção individual do dicionário durante sua enumeração e limpa ao final. Dispose do enumerador também aparece nos caminhos excepcionais. Nenhum reset direto do contador manager+0x14 está presente em DestroyAllEntities. O wrapper Entity.Destroy e efeitos de callbacks ainda devem ser seguidos antes de garantir ausência de mutações indiretas no registro.

## EntityManager.Deserialize

Deserialize37095 (`0x1138ba8`) não começa com DestroyAllEntities ou Clear. A preparação do estado pelo caller continua pendente; não inferir que o método aceite um mundo previamente populado.

No caminho normal:

1. Lê StreamUtil.ReadInt14439 (`0x1c03458`) e grava manager+0x14, o contador usado pela fábrica. Depois de chamadas de formatação/log, lê outro ReadInt como quantidade de entidades e aloca um array temporário com esse tamanho.
2. Para cada entidade, lê StreamUtil.ReadByte14434 (`0x1c0c344`), **estende o byte com sinal** e usa esse seletor em Create virtual slot4, com ID inicial0.
3. Chama entity.Deserialize slot15. Na BaseEntity a implementação é Entity.Deserialize36924 (`0x1131d80`): lê ReadInt para entity+0x28 e ReadBoolean14445 (`0x1c0c6e8`) para byte+0x2c.
4. Chama AddEntity37087 com booleano=false: registra no dicionário e inscreve callbacks (37). Guarda a entidade no array temporário.
5. Depois de registrar **todas** as entidades, percorre o array na mesma ordem e chama slot17, BaseEntity.DeserializeComponents37071. Zera cada entrada temporária após o retorno.
6. Chama manager.OnAfterDeserialize por slot6; a implementação37097 (`0x1138e84`) é RET. MyEntityManager herda esse slot conforme37.

O ID é assim carregado antes do registro e antes da passagem que adiciona componentes aos grupos. Isso fornece um escritor concreto do campo usado no hash (40); não prova ausência de outros escritores. A primeira passagem permite que as entidades já estejam registradas quando os componentes da segunda passagem resolverem referências, mas a resolução concreta deve ser confirmada em cada componente.

## DeserializeComponents

BaseEntity.DeserializeComponents37071 (`0x1136e84`) lê um byte, aplica extensão de sinal e o usa como limite de componentes presentes no stream. Aloca um novo array de componentes com o tamanho manager+0x10, substituindo entity+0x38. Não chama RemoveAll nesse corpo.

Para cada índice de0 até limite-1, lê ReadBoolean. Se false, segue para o próximo. Se true:

- Chama ComponentFactory.Create37429 (`0x1137008`) com o índice.
- No componente obtido, chama slots5,7 e9 nessa ordem; os dois primeiros recebem o stream, o terceiro não recebe argumento explícito adicional. A identificação concreta por tipo ainda deve ser feita.
- Chama entity.Add virtual slot5 com índice e componente. Para BaseEntity, isso grava a posição e notifica os callbacks já inscritos na primeira passagem.

O byte com sinal não deve ser silenciosamente tratado como contador unsigned no port. Limite menor que1 pula o loop após alocar o array. Índices além da capacidade podem atingir falha de limites na adição; não há clamp observado. Uma factory que retorna null segue o caminho excepcional, não ignora o componente.

Esta análise fecha a ordem geral de carregamento, mas ainda não reconstrói o formato de save completo. Endianness, política de EOF, campos dos componentes, referências cruzadas e chamadas superiores precisam ser conferidos antes de escrever um leitor nativo de saves.

## Validação e próximos passos

Exports `build/research/entity-destruction-load.tsv` e `entity-load-followup.tsv`, delimitados por FDE e com coverage.tsv nos diretórios homônimos. Conferidos callbacks, slots15/17 da BaseEntity e slot6 do manager, destinos de leitura e referências genéricas de Dictionary. Cobertura sintática não é execução do save.

Nenhum C++ alterado; os6 testes da rodada39 não foram repetidos. Próximo: Entity.Destroy, StreamUtil e os componentes Animation/Seb/Direction em serialize/deserialize; seguir a preparação anterior a World.Deserialize. Capacidades do HashSet continuam pendentes para integração do ECS. Goal ativo, trabalho estático útil disponível.
