opts = detectImportOptions('stats.csv', 'Delimiter', ';');
df = readtable('stats.csv', opts);

tempo = df.tempo;
tempo_h = tempo / 3600;
Pb_sim = df.prob_bloqueio;
A_oferecido = df.trafego_oferecido;

C = 13;
Pb_erlangB = erlangB(A_oferecido, C);

figure;
plot(tempo_h, Pb_sim, 'b', 'LineWidth', 1.2); hold on;
plot(tempo_h, Pb_erlangB, 'r--', 'LineWidth', 1.5);
xlim([0 24]);
xticks(0:3:24);
xlabel('Tempo [h]');
ylabel('Probabilidade de Bloqueio');
title(['Bloqueio ao longo do tempo']);
legend('Simulação', 'Bloqueio teórico', 'Location', 'best');
grid on;
axis padded;

script_dir = fileparts(mfilename('fullpath'));
out_path = fullfile(script_dir, 'grafico_erlangB_comutador.png');
saveas(gcf, out_path);