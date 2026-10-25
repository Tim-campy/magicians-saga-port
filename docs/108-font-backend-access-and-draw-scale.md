# Acesso à fonte e atualização de escala no desenho

Data: 2026-10-25. Capítulo 4/M3. Pesquisa estática; complementa107.

## Configuração de Graphics

Graphics.get_DefaultUseFontTexture17842 (0x1b5b494) lê o byte estático+0x59. O setter17843 (0x1b5b4ec) aplica AND1 ao argumento e grava esse byte em0x1b5b540, depois da inicialização de classe.

Graphics..cctor17844 (0x1b5b54c) escreve explicitamente false em+0x58 e true em+0x5a (0x1b5b858/85c), mas não foi identificado store explícito em+0x59 no corpo exportado. Isso é consistente com o zero inicial do armazenamento estático; não prova que a configuração permaneça false durante o aplicativo inteiro. Não adotar esse valor como backend definitivo das falas.

A varredura B/BL imediato nos segmentos executáveis não encontrou chamadas ao setter17843 nem a TextLayout.SetFont19215. Isso não cobre indireção, reflexão ou código inline. A varredura de STRB com offset imediato0x59 também encontra campos de outros objetos: o candidato KairoBase.Start16426 em0x1c6c004 é um store em sua própria instância x19, não evidência de alteração de Graphics. Foi exportado e descartado como evidência dessa configuração. A busca por esse offset não equivale a análise de aliases ou de stores mais largos.

## Cópia de fonte não é ligação automática

Graphics.GetFont17958 (0x1b681c8) lê a fonte de Graphics+0x1c8. Se recebe um destino não nulo, chama Font.Copy17792 (0x1b7a4fc) sobre ele com a fonte atual como argumento; caso contrário cria uma Font pelo construtor de cópia17762. Retorna o destino/cópia. Não retorna simplesmente a referência interna.

TextLayout.SetFont19215 (0x1bc362c) primeiro limpa o byte+0x86 em0x1bc3668, então aloca uma Font, chama o construtor de cópia com o argumento e só depois substitui font_+0x18 em0x1bc3684. Se a construção falha, a flag já foi limpa e a referência anterior ainda não foi substituída. Esse método não reconstrói os blocos nesse corpo.

Nenhuma ligação direta GetFont→SetFont foi estabelecida. A fábrica de falas continua ligada ao construtor padrão de TextLayout conforme101. A presença de um produtor de useFontTexture_ em Graphics.Init não autoriza copiar seu backend para toda fonte de TextLayout.

## Draw invalida a formação de blocos pela escala

TextLayout.Draw19206 (0x1bc128c) recebe Graphics. No trecho0x1bc1450..0x1bc1478, após preparação anterior:

1. Lê o float de Graphics+0x5c e compara com TextLayout.scaleRatio_+0x7c.
2. Se iguais, pula a reconstrução.
3. Se diferentes, grava primeiro o novo float em+0x7c.
4. Chama _makeTextBlocks19200 (0x1bbd4c4) em0x1bc1470.
5. Chama _makeTextRubies19201 (0x1bbfde8) em0x1bc1478.

A comparação FCMP/B.EQ trata NaN como diferente; zeros de sinais opostos comparam iguais. A escala fica alterada antes da construção dos blocos. Uma falha nessa construção impede a chamada subsequente dos rubies; as fronteiras de exceção de Draw ainda não foram auditadas integralmente. Não presumir rollback.

O corpo de Draw usa a font_ já presente em TextLayout para consultas. Não há chamada a Graphics.GetFont no corpo exportado. O desenho percorre seus blocos e chama TextBlock.Draw19272 em0x1bc1624; a seleção efetiva de backend por bloco exige seguir esse método e seus dados.

## Evidências e limites

Exports locais em `build/research/font-backend/` (cinco FDEs,2528 bytes) e `font-backend-start/` (dois FDEs,3728 bytes), todos com cobertura sintática integral. ELF SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Endereços descontam a base Ghidra0x100000. Cobertura de bytes não é prova de comportamento observado.

Confiança **confirmed** para acessores, sequência de cópia e recorte de invalidação. Backend efetivo **unknown**. Nenhum C++ alterado ou teste de produto repetido; último build20/20 em106. Próximo: integrar o controle de mudança de escala aos contratos nativos de layout e seguir TextBlock.Draw para a rota concreta de desenho, mantendo localização e cleanup separados conforme104/105.
