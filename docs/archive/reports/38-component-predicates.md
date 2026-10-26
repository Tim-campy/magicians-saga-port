# Predicados de componentes e caminho de leitura dos grupos

Data: 2026-10-25. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`; ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Continuação estática de37, sem execução do jogo. Endereços são VAs ELF; Ghidra soma0x100000.

## ContainsAll e ContainsAny

BaseEntity.ContainsAll37066 (`0x1136b9c`) e ContainsAny37067 (`0x1136c10`) percorrem o array de índices recebido, na ordem original, consultando ponteiros do array this+0x38. Vtable de BaseEntity: slots10/11 contêm0x60012195/0x60012197, resolvendo esses métodos. Nenhum bitset foi presumido.

- ContainsAll retorna false no primeiro componente ausente; retorna true ao alcançar o fim. Array vazio retorna true.
- ContainsAny retorna true no primeiro componente presente; retorna false ao alcançar o fim. Array vazio retorna false.
- Cada índice efetivamente consultado passa por checagem de limites; negativos falham pela comparação unsigned. Não há validação antecipada de toda a lista.
- Índices repetidos não são normalizados. Array de índices null segue a saída excepcional; array vazio não acessa o armazenamento de componentes.

Exemplo verificável: com posição0 ocupada e posição1 vazia, All([1,-1]) retorna false sem consultar -1; Any([0,limite]) retorna true sem consultar limite. Inverter a ordem pode alcançar a falha de índice. Esta ordem faz parte do contrato.

`ComponentSlots::contains_all/contains_any` implementa esses predicados para listas não nulas, recebidas por referência C++. Continua restrito ao armazenamento concreto já portado, sem métodos virtuais personalizados de outras entidades. Não implementa referência nula de array gerenciado nem identidade das exceções IL2CPP.

## Filter.Matches: fluxo confirmado e contexto ainda pendente

Atualização: relatório39 resolve o contexto como IListExtension.IsNullOrEmpty<int> e porta o predicado completo. A pendência descrita abaixo registra o estado desta investigação anterior.

Filter.Matches37161 (`0x113a4dc`) rejeita entidade null antes das demais condições. Usa três referências, em+0x10/+0x18/+0x20. Os nomes metadata são andComponentTypes_, orComponentTypes_, norComponentTypes_. Em cada etapa chama o helper genérico em0x12cd904; se o helper retornar true, pula o predicado correspondente.

Se não pulado, a ordem é:

1. AND: slot10 da entidade, ContainsAll; false encerra com false.
2. OR: slot11, ContainsAny; false encerra com false.
3. NOR: slot11, ContainsAny; true encerra com false; false aceita.

O helper retorna true para referência null. Para referência não nula, resolve uma interface pelo contexto genérico, chama seu método0 e retorna se o resultado é zero. Isso é compatível com teste de coleção vazia, mas a identidade concreta dessa interface/contexto ainda precisa ser decodificada. Portanto não foi implementado um Filter completo a partir dessa hipótese. O comportamento dos dois predicados de BaseEntity acima é independente dessa lacuna e está confirmado diretamente.

Filter..ctor37154 (`0x113a378`) escreve -1 em+0x28. And37155 (`0x113a398`), Or37156 (`0x113a3fc`) e Nor37157 (`0x113a460`) alocam um objeto, escrevem -1 no mesmo campo e guardam o argumento em+0x10/+0x18/+0x20 respectivamente. Não foi implementado encadeamento, hash ou igualdade de filtros: wrappers explícitos IComponentFilter e cache de hash exigem investigação própria.

## Leitura de Subset

Subset.get_entities37108 (`0x113942c`) simplesmente retorna this+0x30. **Não materializa nem retorna o campo+0x38 limpo nas mutações.** Não usar o nome entitiesCache_ isoladamente para presumir que AnimationSystem lê um snapshot.

get_count37107 (`0x11393e4`) lê o inteiro em+0x20 da coleção this+0x30. get_entity37109 (`0x1139434`) retorna null se esse contador for zero; caso contrário encaminha a um helper genérico em0x12af8d4. A seleção concreta desse helper permanece pendente; não foi presumido índice0 ou ordem estável.

Subset..ctor37106 (`0x1138604`) guarda o filtro recebido em+0x28 e cria a coleção guardada em+0x30. A sobrecarga Add37110 (`0x11386c0`), com apenas entidade, consulta IFilter.Matches e, quando true, chama a operação da coleção em0x163589c e limpa+0x38. **Ela não emite a notificação** existente na sobrecarga Add37111 com índice/componente (relatório37). A sobrecarga não deve ser substituída indiscriminadamente pela versão que notifica.

## Validação e próximos passos

Exports `build/research/subset-filters.tsv` (10 FDEs) e `filter-predicates.tsv` (3 FDEs), com cobertura sintática integral nos respectivos coverage.tsv. Incluem getter, overload e helper delimitados por FDE; nenhuma cobertura dinâmica é alegada.

```powershell
tools\build-desktop.cmd
```

Build Windows passou, CTest5/5. Testes de componentes ampliados com listas vazias, repetidas, presença/ausência, ordem de curto-circuito diante de índices inválidos e Change(null). Log local: `build/research/component-predicates-build.log`. Linux e original não executados.

Próximos alvos: contexto genérico do helper de vazio, identidade/semântica das coleções de Subset, wrappers encadeáveis de Filter e enumeração usada pelo AnimationSystem. Sem esses contratos, uma integração completa de grupos poderia alterar ordem de Update e notificações.
