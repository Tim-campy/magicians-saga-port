# Efeitos de progressão: dinheiro, cristais, áreas e missões

Data: 2026-10-25. Evidência estática ARM64 e implementação sobre estado decodificado. VAs sem base Ghidra0x100000. Nenhuma execução do APK ou leitura de saves reais.

## Descoberta e dispatch virtual

O dispatcher unitário AppData.ExecuteScript34379 chama get_isFound38047 antes dos opcodes4/15. O getter em0x116c3a4 retorna state_==1, não state_!=0. Se falso, chama Find(true) por slot8.

Vtables conferidas novamente na metadata do APK: AreaData4502/slot8 resolve para BaseData.Find38052 em0x116c790; QuestData4520/slot8 resolve para QuestData.Find38509 em0x1172f0c.

- Área: Find grava state=1 e new=true. No opcode4, uma área já no estado1 mantém seu flag new anterior, porque Find não é chamado.
- Missão: o override ignora o argumento e só escreve state=1/new=true quando state era0. Estados2 ou negativos não são convertidos, embora isFound seja falso.

Não generalizar descoberta de missões para a regra de estoque do relatório70. Índices inválidos e registros null interrompem o efeito, sem ignorar silenciosamente a linha.

## Saldos e larguras numéricas

Exports em `build/research/progression-effects/`, selecionados por método e intervalo FDE:

| Método | VA | Aritmética observada |
| --- | --- | --- |
| AddMoney34334 | 0x110946c | Soma argumento int32 estendido com sinal ao saldo int64, com wrap64, sem clamp. |
| SubMoney34336 | 0x11096e0 | Subtrai int32 estendido com sinal, grava resultado e, se negativo, substitui por zero. |
| AddCrystal34340 | 0x11099f0 | Mesmo padrão de soma, em outro slot. |
| SubCrystal34341 | 0x1109ac4 | Ajusta o argumento conforme estado descrito abaixo; subtrai em64 bits e aplica piso zero. |

Dinheiro fica em AppData+0x198 → array+0x28, elemento0; cristais no elemento1. Os helpers de SecureLong acessam esses valores. A implementação nova opera sobre seus valores decodificados; não replica armazenamento protegido, alocação do runtime ou falhas intermediárias desses wrappers.

Para os opcodes1/2, argumento não negativo chama adição. Negativo é negado **em32 bits** antes de entrar no método de subtração. INT_MIN continua INT_MIN após a negação, podendo aumentar o saldo na subtração; não trocar isso por abs64 ou saturação intuitiva.

### Condição adicional da subtração de cristais

SubCrystal lê um SecureInt no array de sistema AppData+0x190 → +0x20, usando o índice da variável estática AppData+0x314. A resolução do descritor e dos offsets identifica essa variável como **SYSSAVEI_YUGAMI**, field20598/type4165.

Se o valor lido é10000, conserva o argumento. Caso contrário, multiplica o argumento por10000 com wrap32. Só então estende com sinal e subtrai do saldo int64. A origem, inicialização e significado completo desse valor do save ainda não foram fechados. O módulo exige seu valor explicitamente; não presume10000, não remove a condição e não altera dados originais.

## Substituição de argumentos

Antes do switch, o dispatcher clona a linha. Nos elementos1 em diante, valores de INT_MIN até INT_MIN+99 são substituídos pelo elemento de argi indexado por `uint32(value) XOR 0x80000000`. A substituição é uma passagem: um valor substituído que também seja sentinela não é resolvido novamente. O opcode em posição0 não é substituído. A linha original permanece intacta.

O helper nativo `resolve_event_arguments` implementa essa transformação; null ou índice inválido em argi falham. Diagnósticos condicionais de Config.DEBUG não foram reproduzidos. Essa transformação pertence ao dispatcher unitário: opcode10 interceptado pelo executor da matriz continua seguindo o caminho próprio do relatório82.

## Implementação e integração verificada

`src/core/gameplay/event_progression.{hpp,cpp}` fornece o estado decodificado e `apply_event_progression` para opcodes1/2/4/15. Os demais retornam explicitamente “não tratado”, para que o host os encaminhe à implementação correta. Não existe fallback que simule falas, telas ou stamina.

`tests/test_event_progression.cpp` integra esses efeitos com `event_flow`: crédito, espera, descoberta de missão com argumento capturado e débito. O teste muda o array de argumentos do chamador durante a espera e confirma que a retomada usa a cópia original. Outros casos verificam estados de descoberta, índices/null, sentinelas, wrap32/64, piso zero e ambos os ramos da condição de cristais.

Comando `tools\build-desktop.cmd`: **12/12 testes CTest passaram em Windows**, sem warnings de compilação encontrados no log `build/research/event-progression-build.log`. Não houve build Linux, execução gráfica ou comparação com o APK.

## Próximo passo

Resolver e integrar efeitos dos opcodes8/12/31 (falas, formulários de evento e stamina), identificação/resolução de registros e carregamento dos dados. A origem do valor SYSSAVEI_YUGAMI permanece pendente antes de construir o estado inicial real. Essa integração de teste é avanço funcional dos contratos, não uma campanha ou loop jogável completo.
