# Armazenamento e notificações de componentes

Data: 2026-10-25. APK 1.4.0 SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`; ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Evidência estática; nenhum APK executado.

## Escopo e evidência

Atualização posterior: o relatório37 resolve a fábrica de MyEntityManager e a inicialização com35 posições para o seletor0. As limitações de integração com grupos/pools abaixo continuam vigentes; a pendência da fábrica descrita nesta rodada foi fechada para esse caminho concreto.

Contrato concreto de BaseEntity, tipo metadata4371, com array de componentes em this+0x38. Não é uma implementação completa de EntityManager, pools ou grupos ECS. Endereços abaixo são VAs ELF; os exports Ghidra somam 0x100000.

| Método metadata | VA ELF | Observação |
|---|---|---|
| BaseEntity..ctor37058 | 0x1136824 | Cria array com quantidade recebida explicitamente |
| BaseEntity..ctor37059 | 0x11368c8 | Delega ao construtor base; não aloca o array neste corpo |
| get_type37060 | 0x1136934 | Getter concreto |
| Add37061 | 0x113693c | Ignora posição já ocupada |
| RemoveAll37062 | 0x11369d4 | Varredura crescente com chamadas de Remove |
| Remove37063 | 0x1136a2c | Limpa antes do callback |
| Change37064 | 0x1136a94 | Troca antes do callback; delega a Add se ausente |
| Contains37065 | 0x1136b64 | Presença significa ponteiro não nulo |
| GetComponent37068 | 0x1136c84 | Retorna ponteiro após checagem de índice |
| Entity.OnAddComponent36927 | 0x1131e00 | Delegate em entity+0x10 |
| Entity.OnRemoveComponent36928 | 0x1131e2c | Delegate em entity+0x18 |
| Entity.OnChangeComponent36929 | 0x1131e58 | Delegate em entity+0x20 |
| World.CreateEntity36992 | 0x1133568 | Encaminha para EntityManager com segundo argumento zero |
| EntityManager.CreateEntity37082 | 0x11375e4 | Usa fábrica virtual no slot4 do manager |

Seleções reproduzíveis: `build/research/entity-components.tsv`, `entity-component-followup.tsv` e `entity-create.tsv`; exports em diretórios homônimos. Os14 FDEs têm cobertura sintática integral registrada em coverage.tsv. Isso não prova alcance em runtime. O último corpo despacha a fábrica virtual por class+0x178 antes do registro da entidade. Ainda é necessário resolver a implementação concreta dessa fábrica para generalizar este contrato a todos os produtores de entidade. Entradas abstratas de vtable não devem ser decodificadas como método0.

## Contrato confirmado

- Add consulta presença primeiro. Se ocupado, retorna sem troca ou notificação. Se vazio, grava o argumento, inclusive null, e chama OnAddComponent. Portanto Add(null) pode notificar enquanto Contains continua false.
- Change em posição vazia delega a Add. Em posição ocupada, captura o ponteiro antigo, grava o novo e notifica com ambos. Ponteiros iguais ainda notificam; trocar por null gera Change, não Remove.
- Remove vazio não faz nada. Remove ocupado captura o valor, zera a posição e notifica com o valor antigo.
- RemoveAll percorre índices em ordem crescente, consultando o estado vivo. Um callback que preenche uma posição posterior faz essa posição ser removida na mesma passagem. Reinserção na posição atual ou anterior permanece, pois não há retorno a índices visitados.
- Os wrappers de notificação chamam o delegate quando presente, passando entidade, índice e ponteiros correspondentes. A mutação já ocorreu. Não há rollback local se o callback falhar; RemoveAll interrompe com as alterações anteriores preservadas.
- Índice é verificado contra o tamanho do array; negativos também falham. O original também valida compatibilidade do objeto não nulo com o tipo de elemento do array.

## Implementação portátil e limites

`src/core/component_slots.hpp/.cpp` implementa esse armazenamento em C++17: vetor de ponteiros emprestados, tamanho fixado explicitamente na construção, callbacks síncronos e identidade estável. Component é a fronteira de tipos portátil, não uma reprodução do layout IL2CPP. Componentes e observer precisam permanecer válidos durante operações e callbacks. O módulo não toma posse nem clona ou libera componentes. As operações de armazenamento não alocam; callbacks podem fazer trabalho próprio.

Não foram portados GC, pool, compatibilidade dinâmica de arrays IL2CPP, construção com array ausente, subclasses que sobrescrevam os métodos virtuais ou inscrição em grupos. Exceções C++ de índice/tamanho representam falhas de contrato, sem prometer identidade com exceções gerenciadas. Não foi ligado ainda ao módulo de animação ou a uma cena. Esses limites evitam transformar o comportamento de uma classe concreta em uma regra presumida de todo o ECS.

## Validação

`tests/test_component_slots.cpp` verifica duplicata, null, troca pelo mesmo ponteiro, argumentos e visibilidade dos callbacks, ordem crescente, reinserção posterior/atual/anterior, interrupção por exceção sem rollback, índices inválidos e array vazio. Revisão independente conferiu o módulo contra os exports sem encontrar divergência nesse escopo.

```powershell
tools\build-desktop.cmd
```

Resultado Windows x64: build bem-sucedido, CTest5/5 passou, incluindo regressões de animação e leitores Jar/OPT/SEB. Log local `build/research/component-slots-build.log`. Linux e comparação com o jogo não executados. Próximo alvo: fábrica concreta do manager, callbacks de manutenção de grupos e propriedade/reutilização dos componentes antes de integrar o ECS.
