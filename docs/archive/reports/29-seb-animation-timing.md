# Avanço estático de animações SEB

Data: 2026-10-25. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`; ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Esta investigação não executa o jogo nem implementa uma frequência aproximada de animação.

## Método

Quatorze intervalos FDE foram exportados por `ExportBoundedInstructions.java`, sem decompilador, com cobertura sintática integral: seleções `build/research/seb-timing.tsv` e `seb-timing-followup.tsv`, saídas em diretórios homônimos e `coverage.tsv`. Os endereços no relatório são VAs ELF; os exports somam `0x100000`.

```powershell
& C:/Ghidra/support/analyzeHeadless.bat build/research/ghidra wizard_il2cpp_fde -process libil2cpp.so -noanalysis -scriptPath tools/ghidra -postScript ExportBoundedInstructions.java build/research/seb-timing.tsv build/research/seb-timing -max-cpu 1
```

Uma varredura dos words ARM64 alinhados em PT_LOAD executável encontrou 28 BL/B imediatos para os cinco alvos Frame, PushThisFrameRender, ResetThisFrameRender, ThisFrameRenderAnimation e set_AutoAnimation. Saída: `build/research/seb-timing/direct-references.json`. Não cobre indireções ou stores inline, nem prova alcance; nenhum destino direto ao setter foi encontrado. Chamadores são associados por FDE, não por intervalo arbitrário entre símbolos.

## Campos corroborados por getters e setter

| Campo | Evidência ELF | Significado comprovado |
| --- | --- | --- |
| Seb instance+0x18 | GetCurFrame18991 `0x1bb16ac`, SetCurFrame18992 `0x1bb16b4` | Frame corrente, inteiro32; setter grava sem normalizar |
| Seb instance+0x1c | GetMaxFrame18993 `0x1bb16bc` | Máximo usado no módulo, inteiro32 |
| Seb static+0x14 | get_AutoAnimation18969 `0x1baf3e4`, setter18970 `0x1baf444` | Flag global AutoAnimation, setter usa argumento &1 |
| Seb instance+0xa0 | Push/Reset abaixo | Flag de registro do objeto na lista de desenho |
| Seb instance+0xa4 | Push abaixo | TotalFrames do último registro |

O bloco estático Seb é acessado pelo GOT ELF `0x272f880`, através de classe+0xb8. O mesmo slot nos getters/setter e demais métodos sustenta sua identidade. Não confundir flag global de animação com propriedade independente de cada sprite.

## Avanço manual: Frame18994

Em `0x1bb1748–0x1bb1778`, o método retorna sem alterar o frame se AutoAnimation estiver ligado **ou** se IApplication.static+0x1c estiver desligado. Esse último byte foi visto sendo ligado em Update no relatório24; seu ciclo completo de escrita ainda não está fechado.

No caminho aceito, `0x1bb177c–0x1bb178c` faz ADD32 de 1 ao frame, SDIV32 pelo máximo e MSUB32 para obter resto, armazenando em instance+0x18. Para frame não negativo sem overflow e máximo positivo, equivale a `(frame+1) % maxFrame`. **Não há teste de maxFrame positivo nesse trecho**; não copiar literalmente para `%` C++ sem política de equivalência para zero, sinal e overflow. A instrução ARM64, não aritmética C++ indefinida, é a evidência de referência.

Essa chamada aparece em AnimationSystem.Update, RoomForm.Update, SubForm.Update e outros formulários; o inventário de destinos não estabelece quantas chamadas acontecem por segundo.

## Registro automático: PushThisFrameRender19073

Regras observadas:

1. AutoAnimation desligado ou instance.byte+0xa0 já ligado: retorna false (`0x1bb3840–0x1bb3854`). A segunda condição evita inserir duas vezes o mesmo objeto entre resets; não deduplica imagens por nome.
2. Obtém IApplication singleton.TotalFrames (+0x58) e subtrai instance+0xa4. Se o resultado **signed32 >=2**, grava frame zero (`0x1bb38b0–0x1bb38c4`). O teste mede contador do framework, não milissegundos ou atualizações de gameplay. Diferença e overflow conservam semântica de32bits.
3. Usa coleção Seb.static+0x8 e quantidade ativa static+0x10. Se necessário aumenta a quantidade de slots da coleção; grava o objeto no índice ativo (`0x1bb38e0–0x1bb39a8`). As chamadas genéricas não foram nomeadas apenas pela aparência.
4. Incrementa quantidade ativa, liga instance+0xa0, grava TotalFrames em instance+0xa4 e retorna true (`0x1bb39ac–0x1bb3a1c`).

O construtor estático Seb19079 (`0x1bb85dc`) cria a coleção em static+0x8. Não contém escrita explícita em AutoAnimation+0x14; nenhum valor de configuração do jogo para esse flag foi provado nesta rodada. Ausência de BL ao setter não exclui stores inline/reflexão.

## Avanço automático e reset

**ThisFrameRenderAnimation19075** (`0x1bb82b4`): com AutoAnimation ligado, percorre índices `[0, static+0x10)`, obtém cada Seb da coleção e testa **maxFrame >=1** (`0x1bb839c–0x1bb83a4`). Só então aplica incremento e resto signed32 ao frame (`0x1bb8428–0x1bb8438`). Ao contrário de Frame manual, há guarda positiva do divisor. Não limpa quantidade ativa nem flag de registro.

**ResetThisFrameRender19074** (`0x1bb8190`): com AutoAnimation ligado, percorre o mesmo conjunto ativo, grava byte zero em cada instance+0xa0 (`0x1bb828c`) e zera a quantidade static+0x10 (`0x1bb829c`). Não modifica frames nem chama Clear na coleção; slots podem continuar retendo referências além da quantidade ativa. Não assumir que libera os objetos a cada passagem.

No caminho de OnGUI já estudado, ThisFrameRenderAnimation é chamado em `0x1b84090`, condicionado a IApplication.static+0x1c; Reset vem depois em `0x1b840b0`, antes do desenho principal. Portanto a lista avançada é a acumulada antes desse reset. Não assumir avanço automático imediato em toda chamada Draw, nem associar um evento GUI a um tick fixo.

Três sobrecargas Draw (19024/19025/19026) foram conferidas integralmente: chamam Push e depois leem o frame corrente para encaminhar ao desenho. Exemplos: Push em `0x1bb425c`, leitura +0x18 em `0x1bb4260`; Push em `0x1ba8bd8`, leitura em `0x1ba8bdc`. O retorno booleano de Push não condiciona o desenho nesses wrappers; Draw ainda prossegue quando Push retorna false.

## Existe ainda um contador por entidade

`ecs.AnimationSystem.Update`37433 (`0x1141bc0`) não se limita ao Seb compartilhado:

- Primeiro percorre a coleção em this+0x28 e arrays internos em +0x18, chamando Seb.Frame para cada entrada não nula (`0x1141c24–0x1141cb4`).
- Depois percorre entidades. Entity.get_seb36886 (`0x1131018`) fornece um componente com índices em +0x10/+0x14 e contador próprio em +0x18. Estes offsets pertencem ao **componente**, não ao objeto Seb, apesar da coincidência numérica.
- Seleciona um Seb pelos dois índices e lê seu maxFrame (`0x1141d40–0x1141d90`). Campos negativos são filtrados; verificações de limite e null estão no código.
- Lê Entity.get_animation36763 (`0x112c3f0`), soma seu inteiro+0x10 ao contador do componente (`0x1141da8–0x1141dc4`).
- Se animation+0x14 != -1 e o contador alcança maxFrame, chama AnimationSystem.RestoreAnimation37438 (`0x1141dd8–0x1141e04`). O significado desse campo e os efeitos de RestoreAnimation continuam abertos.
- Obtém novamente o componente e aplica resto por maxFrame (`0x1141e08–0x1141e24`). Isso ocorre após a eventual restauração; o divisor foi guardado antes dela. Não substituir por loop simples sem seguir esse efeito.

É incorreto fundir automaticamente o frame compartilhado de Seb com o contador de cada entidade ou assumir que ambos avançam por1.

## Validação e próximo passo

Todos os14 FDEs tiveram bytes integralmente cobertos pelo export; SHA e intervalos constam dos relatórios. Getters, setters, guards e wrappers foram confrontados diretamente com assembly. Isso valida a descrição estática, não frequência real ou apresentação visual. Nenhuma alteração de renderer ou simulação foi feita.

Próximos alvos: RestoreAnimation e campos dos componentes; ligação de AnimationSystem.Update ao evento lógico; escritores de AutoAnimation e IApplication.static+0x1c. Há trabalho estático concreto antes de qualquer necessidade exclusiva de teste dinâmico.
