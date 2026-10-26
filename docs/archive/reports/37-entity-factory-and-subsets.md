# Fábrica concreta e manutenção dos grupos ECS

Data: 2026-10-25. Continuação do relatório35. Mesmo APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead` e ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Análise estática, sem execução Android. VAs abaixo não incluem a base Ghidra0x100000.

## Fábrica usada pelo mundo

World..cctor36958 (`0x11332b0`) aloca MyEntityManager e passa **0x23=35** ao construtor em `0x1133320`; depois passa esse manager ao construtor World em `0x1133334`. World..ctor36984 (`0x11333ac`) armazena o argumento em World+0x20. A identidade da classe alocada foi conferida: GOT0x2730cb8 → relocação0x27c8790 → referência0x20006f1b → native type14221 → metadata4445, ecs.MyEntityManager.

MyEntityManager..ctor37426 (`0x1141a40`) é uma única tail call para EntityManager..ctor37078 (`0x1137424`). Este grava a quantidade recebida em manager+0x10 e zero no contador+0x14, antes de criar suas coleções.

EntityManager.CreateEntity37082 (`0x11375e4`, relatório35) chama slot4 da fábrica. Vtable de MyEntityManager começa em58879, tem7 entradas e slot4=0x60012467→37427. **MyEntityManager.Create37427 (`0x1141a44`) é uma tail call para EntityManager.Create37083 (`0x11378d0`)**. Na classe base, vtable58298 slot4=0x600121b7→37083.

Create37083 retorna null se o primeiro argumento explícito, seletor de tipo, não for zero. Se zero, lê manager+0x10, aloca **BaseEntity** e chama seu construtor37058 com ID recebido, quantidade de posições e manager. Identidade confirmada por GOT0x2730e30 →0x27c5008→0x20004945→native type9378→metadata4371. O construtor aloca o array conforme relatório35.

Assim, a cadeia de inicialização examinada sustenta BaseEntity com35 posições para o tipo0. Não prova que toda entidade de qualquer caminho, desserialização ou subclasse use essa mesma construção. CreateEntity ainda trata retorno nulo como erro e registra o resultado não nulo; não substituir por uma factory que aceite silenciosamente tipos desconhecidos. O contrato de alocação/reuso dos IDs, incluindo limite e busca de IDs livres, ainda não foi portado.

## Registro de callbacks

EntityManager.AddEntity37087 (`0x1137758`) primeiro consulta o booleano recebido: se falso, adiciona a entidade à coleção manager+0x18 usando o campo entity+0x28 como chave. Depois, em ambos os caminhos, inscreve três delegates na entidade, nesta ordem:

| Evento | Callback | Evidência |
|---|---|---|
| OnComponentAdded | OnEntityComponentAdded37088 | GOT0x2730e18→0x27dd938→0x600121c1; inscrição36899 em0x1137860 |
| OnComponentRemoved | OnEntityComponentRemoved37089 | GOT0x2730e28→0x27dd948→0x600121c3; inscrição36901 em0x113788c |
| OnComponentChanged | OnEntityComponentChanged37090 | GOT0x2730e20→0x27dd940→0x600121c5; tail call36903 em0x11378c8 |

O corpo não contém uma passagem inicial de todos os componentes para sincronizar os grupos. Não interpretar o booleano como desativação dos callbacks: ele controla o trecho inicial de registro na coleção, enquanto as inscrições continuam.

## Adicionar/remover reavalia o filtro; Change segue outro caminho

OnEntityComponentAdded37088 (`0x1137d60`) e OnEntityComponentRemoved37089 (`0x11380b0`) enumeram a coleção obtida a partir de manager+0x20. Para cada Subset chamam **Subset.Add37111** (`0x1137f00`) com entidade, índice e componente da notificação. Se o resultado for false, chamam **Subset.Remove37112** (`0x1138018`) com os mesmos argumentos. Os dois handlers usam esta mesma sequência, apesar dos nomes diferentes.

Subset.Add chama o método0 da interface da referência this+0x28. Identidade conferida: GOT0x2730e90→0x27c7170→0x2000615d→native type12462→metadata4393 **IFilter**, cujo único método é **Matches37174**. O argumento é a entidade, já alterada pela operação de BaseEntity. O resultado de Matches determina o retorno booleano de Add.

Quando Matches é true, Add chama a operação da coleção this+0x30 em0x163589c, limpa this+0x38 e, se presente, chama o delegate this+0x10. O resultado da operação da coleção não é testado antes da notificação. Ainda falta resolver suas instanciações genéricas para fechar semântica de duplicatas e ordem de iteração; não inferir que a notificação representa necessariamente uma nova inclusão.

Subset.Remove chama a operação da coleção em0x1634fe8. Só notifica this+0x18 se o retorno indicar sucesso. Limpa this+0x38 **depois** do callback, inclusive no caminho de remoção sem sucesso. Portanto a ordem da invalidação difere de Add; uma exceção no callback pode impedir essa limpeza posterior. Os campos metadata de Subset incluem filter_, entities_, entitiesCache_ e dirty_, mas a correspondência integral de offsets/tipos deve ser validada antes de implementar o cache.

OnEntityComponentChanged37090 (`0x1138250`) enumera a mesma origem e invoca diretamente o delegate Subset+0x20 quando não nulo, com entidade, índice, componente antigo e novo. **Não chama Matches, Add ou Remove neste corpo.** Subset.ChangeComponent37113 (`0x11383ec`) também é apenas um encaminhamento para esse delegate.

Isso importa para Change(null): relatório35 prova que a posição fica vazia, mas esse caminho de notificação não reavalia associação aos grupos. Não acrescentar automaticamente atualização de membership ao portar Change. Efeitos dos assinantes e contratos dos filtros ainda precisam ser investigados antes de integrar o ECS completo.

## Reprodução, validação e próximos passos

Exports novos: `build/research/entity-factory.tsv` (5 FDEs,700 bytes), `entity-registration.tsv` (5 FDEs,1872 bytes) e `subset-mutations.tsv` (3 FDEs). Todos têm cobertura sintática integral registrada nos respectivos diretórios. Exemplo:

```powershell
& C:/Ghidra/support/analyzeHeadless.bat build/research/ghidra wizard_il2cpp_fde -process libil2cpp.so -noanalysis -scriptPath tools/ghidra -postScript ExportBoundedInstructions.java build/research/entity-factory.tsv build/research/entity-factory -max-cpu 1
```

Foram conferidos assembly, limites FDE, relocação ELF, entradas de vtable dentro da contagem declarada e nomes de tipos/métodos da metadata. Cobertura sintática não prova alcance em runtime. Nenhum C++ alterado nesta rodada; não foi repetido o build5/5 já aprovado. Próximos alvos: métodos concretos IFilter.Matches, instanciações das coleções de Subset, cache/dirty e descarte dos delegates. Esses passos ainda podem ser feitos estaticamente.
