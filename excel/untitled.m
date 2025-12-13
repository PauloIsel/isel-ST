% Lê o ficheiro CSV com separador ';'
opts = detectImportOptions('stability_timeseries.csv', 'Delimiter', ';');
data = readtable('stability_timeseries.csv', opts);

% Identifica a coluna de tempo (assumo que é a primeira)
tempo = data{:, 1};

% Tenta encontrar a coluna de ocupação
% p_util é o nome típico
if any(strcmp(data.Properties.VariableNames, 'p_util'))
    ocupacao = data.p_util;
else
    % alternativa: procura nomes semelhantes como 'util' ou 'ocup'
    cols = data.Properties.VariableNames;
    idx = find(contains(lower(cols), 'util') | contains(lower(cols), 'ocup'), 1);
    ocupacao = data{:, idx};
end

% Plot
figure;
plot(tempo, ocupacao, 'LineWidth', 1.2);
xlabel('tempo');
ylabel('ocupação (p\_util)');
title('Ocupação ao longo do tempo');
grid on;

saveas(gcf, 'grafico_ocupacao.png');
