csv_path = 'stability_timeseries.csv';
opts = detectImportOptions(csv_path, 'Delimiter', ';');
df = readtable(csv_path, opts);

time_col = df.Properties.VariableNames{1};
tempo = df.(time_col);
tempo_h = tempo / 3600;

if any(strcmp(df.Properties.VariableNames, 'p_util'))
    ocupacao = df.p_util;
else
    cols = df.Properties.VariableNames;
    idx = find(contains(lower(cols), 'util') | contains(lower(cols), 'ocup'), 1);
    ocupacao = df{:, idx};
end

figure;
plot(tempo_h, ocupacao, 'LineWidth', 1.2);
xlim([0 24]);
xticks(0:3:24);
xlabel('Tempo [h]');
ylabel('Ocupação (p\_util)');
title('Ocupação ao longo do tempo');
grid on;
axis padded;

script_dir = fileparts(mfilename('fullpath'));
out_path = fullfile(script_dir, 'grafico_ocupacao_rede.png');
saveas(gcf, out_path);
