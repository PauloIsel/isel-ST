csv_path = 'stability_timeseries.csv';
opts = detectImportOptions(csv_path, 'Delimiter', ';');
df = readtable(csv_path, opts);

tempo = df.tempo;
tempo_h = tempo / 3600;

if any(strcmp(df.Properties.VariableNames, 'CarriedErlangs'))
    erlangs = df.CarriedErlangs;
else
    idx = find(contains(lower(df.Properties.VariableNames), 'erlang'), 1);
    erlangs = df{:, idx};
end

C = 7;
B = erlangB(erlangs, C);

figure;
plot(tempo_h, B, 'LineWidth', 1.5);
xlim([0 24]);
xticks(0:3:24);
xlabel('Tempo [h]');
ylabel('Probabilidade de Bloqueio B');
title(['Erlang B ao longo do tempo (C = ' num2str(C) ')']);
grid on;
axis padded;

script_dir = fileparts(mfilename('fullpath'));
out_path = fullfile(script_dir, 'grafico_erlangB_rede.png');
saveas(gcf, out_path);