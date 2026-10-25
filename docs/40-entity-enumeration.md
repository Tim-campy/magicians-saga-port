# Comparador e ordem de visita das entidades

Data: 2026-10-25. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`; ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Análise estática em continuação de39. Endereços abaixo são VAs ELF; exports adicionam0x100000.

## Comparador efetivamente fornecido ao HashSet

Subset..ctor37106 lê o objeto estático da classe referenciada pelo GOT0x2730c80 e o fornece ao construtor de HashSet<Entity> (38/39). A referência codificada0x20009a49 resolve native type19748, endereço0x28404c8, kind0x12 e definição4355, EqualityComparer. A metadata registra o campo estático instance_.

EqualityComparer..cctor36943 (`0x1132d24`) aloca o comparador e grava sua referência no primeiro campo estático. Equals36944 (`0x1132d88`) compara diretamente os dois ponteiros e retorna igualdade. GetHashCode36945 (`0x1132d94`) lê o inteiro entity+0x28 e falha se receber null. O construtor36946 (`0x1132d80`) apenas encaminha à base. Slots4/5 da vtable58189 são0x600120a1/0x600120a3, resolvendo Equals/GetHashCode concretos.

Consequências: duas instâncias distintas com mesmo ID podem coexistir no conjunto; igualdade não é por ID. O hash vem do campo usado como chave no registro da entidade (37), e não do endereço do objeto. Não substituir isso por comparação estrutural dos componentes.

InternalHash em0x16366c0 trata null como hash0 antes de chamar o comparador. Para objeto não nulo, chama método1 da interface do comparador e aplica máscara **0x7fffffff** ao resultado. Assim, a falha de GetHashCode(null) não é alcançada nesse caminho interno. O comparador não garante sozinho a imutabilidade do ID; os escritores desse campo ainda precisam de auditoria antes de assumir estabilidade durante toda a vida de uma entidade.

## O AnimationSystem usa este enumerador

AnimationSystem.Update37433 (`0x1141bc0`, export existente em seb-timing-followup/) lê Subset+0x30 em0x1141cc0, chama GetEnumerator em0x1141ce8 e MoveNext em0x1141d04. Referências genéricas confirmadas usando as tabelas de39:

| GOT | Referência | Especificação | Método |
|---|---|---|---|
| 0x2731320 | 0xc001aab7 | 54619=(49673,1063,-1) | HashSet<Entity>.GetEnumerator |
| 0x2731310 | 0xc001a623 | 54033=(49694,1063,-1) | Enumerator.MoveNext |
| 0x2731308 | 0xc001a621 | 54032=(49693,1063,-1) | Enumerator.Dispose |

GetEnumerator0x1635290 encaminha ao construtor0x157ad38. Este guarda o conjunto em enumerator+0, índice0 em+8, versão corrente do conjunto em+0xc e current=null em+0x10.

MoveNext0x157ad64 primeiro compara a versão capturada com collection+0x38. Divergência vai para construção/lançamento de exceção; não continua sobre snapshot. Depois varre o array de slots collection+0x18 por índices crescentes, começando em enumerator+8, enquanto índice<collection+0x24. Slots têm16 bytes: hash em+0, próximo em+4, referência em+8 (mais cabeçalho do array). Hash negativo indica posição removida e é pulado. Ao encontrar posição ativa, grava o ponteiro em current, avança o índice e retorna true. Ao esgotar, limpa current, fixa índice=limite+1 e retorna false.

Portanto a visita **não segue a ordem dos buckets, nem ordena por ID**. Segue as posições internas ocupadas. Alterações efetivas no conjunto entre chamadas invalidam a enumeração pela versão. O Add duplicado e Remove ausente de39 não incrementam version; callbacks de Subset ainda podem executar outras alterações, então não presumir que uma notificação sem inserção seja sempre inofensiva à enumeração.

## Reuso e crescimento preservam outra ordem que uma lista simples

AddCore0x1636328 (39) reutiliza primeiro a cabeça da lista de posições livres em collection+0x28; na ausência de posições livres, usa collection+0x24 e incrementa esse limite. Remove0x1634fe8 marca o slot removido com hash=-1, limpa sua referência e insere a posição na cabeça da lista livre. Quando a contagem chega a zero, zera o limite e fixa cabeça livre=-1.

Exemplo derivado do fluxo, sem alegar execução: inserir A,B,C ocupa posições0,1,2; remover B libera1; inserir D reutiliza1. A próxima enumeração visita **A,D,C**, não A,C,D. Remoções múltiplas formam uma pilha de posições reutilizáveis. Um vetor com append ou std::unordered_set não preservaria automaticamente essa regra.

IncreaseCapacity0x163614c busca um novo tamanho por helper e encaminha a SetCapacity0x1636214. Este aloca novo array de slots, chama o helper de cópia0x1e84260 com índices de origem/destino0 e quantidade=limite, depois reconstrói os encadeamentos dos buckets usando os mesmos índices. Não há compactação/reordenação de slots nesse caminho. A política numérica de capacidades e o helper de inicialização ainda não foram recuperados completamente; nenhum conjunto substituto foi implementado nesta rodada.

## Validação e próximos passos

Seleções locais `build/research/entity-enumeration.tsv`, `entity-enumeration-followup.tsv` e `entity-set-capacity.tsv`:10 FDEs,1092 bytes, cobertura sintática integral. Conferidos assembly, vtable, identidade do comparador e referências genéricas do próprio AnimationSystem. A ordem ilustrada é consequência estática dos corpos, não captura de runtime.

Nenhum C++ alterado; não houve repetição dos6 testes aprovados na rodada39. Próximos alvos: capacidades/Initialize, escrita de IDs, descarte e Clear, demais consumidores do conjunto e requisitos de serialização. Só então integrar um armazenamento de entidades que preserve IDs, posições, versão e notificações. A ausência de Android não bloqueia essas tarefas estáticas.
