close all, clear all; clc;

arduinoSerial = serialport('COM7', 9600); % Ajuste COM e baudrate
flush(arduinoSerial); % Limpa buffer da porta serial
dev = visadev("USB0::0x0957::0x1798::MY56041566::0::INSTR");

disp("Em espera...");

escala = {'Hz','kHz','MHz','GHz'};
watchdogThreshold = 0.4;
noiseLevel = 40e-3;

% Ignorar primeiras 7 mensagens
nIgnorar = 7;
contador = 0;
contadorD = 0;
contadorR = 0;

while true
    if arduinoSerial.NumBytesAvailable > 0
        linha = strtrim(readline(arduinoSerial));  % remove espaços/extras

        contador = contador + 1;

        % Ignorar as primeiras 7 mensagens
        if contador < nIgnorar
            continue;
        elseif contador == nIgnorar
            fprintf('Osciloscópio a ligar\n')
            writeline(arduinoSerial, "WAIT");
            pause(0.5);
            writeline(dev, ":SINGLE");
        end

        if contains(lower(linha), "relampago") || contains(lower(linha), "disturbio")
            disp('Sinal captado')
            try
                writeline(arduinoSerial, "STOP");
                writeline(dev, ":STOP");
                writeline(dev, ":WAV:SOUR CHAN1");
                writeline(dev, ":WAV:FORM BYTE");
                writeline(dev, ":WAV:MODE NORM");
                writeline(dev, ":WAV:PRE?");
    
                pre = readline(dev);
                params = str2double(strsplit(pre, ','));
    
                tInc  = params(5); % Tempo por unidade
                tZero = params(6); % Zero t
                tRef  = params(7); % Referencial t
                VInc  = params(8); % Volts por unidade
                VZero = params(9); % Zero V
                VRef  = params(10); % Referencial V
                
                writeline(dev, ":WAV:DATA?");
                raw = read(dev, 100000, "uint8");
                
                % Remover cabeçalho dos números
                hashIdx = find(raw == '#', 1);
                numDigits = str2double(char(raw(hashIdx+1))); 
                numBytes = str2double(char(raw(hashIdx+2 : hashIdx+1+numDigits)));
    
                %Converter para 'double'
                dataStart = hashIdx + 2 + numDigits;
                data = double(raw(dataStart : dataStart + numBytes - 1));
        
                %Converter para a tensão real
                volts = (data - VRef) * VInc + VZero;
        
                %Converter para o tempo real
                t = ((0:length(volts)-1) - tRef) * tInc + tZero;
                Fs = round(1 / mean(diff(t)));
                pkV = max(abs(volts));
        
                % Gráfico do sinal obtido
                figure;
        
                plot(t*1e6, volts), grid on
                xlabel("Tempo (us)")
                ylabel("Tensão (V)")
                xlim([min(t)*1e6, max(t)*1e6])
                title("Sinal captado")
        
                if pkV < noiseLevel
                    ylim([-0.7,0.7])
                end
    
                % Análise por wavelets
                [cwtCoeffs, freq] = cwt(volts, Fs); % Obter os coeficientes de wavelet
                mag = abs(cwtCoeffs); % Obter a magnitude
                magFiltrada = mag;
                magFiltrada(mag < noiseLevel) = NaN; % Ignorar as magnitudes abaixo do nível de ruído
        
                indMax = NaN(1, size(magFiltrada, 2));  % Inicia vetor com NaN
    
                % Encontrar o índice da frequência dominante para cada instante de tempo
                for i = 1:size(magFiltrada, 2)
    
                    col = magFiltrada(:, i);
                    if all(isnan(col))
                        continue;  % Ignora colunas sem dados válidos
                    end
    
                    [~, ind] = max(col);
                    indMax(i) = ind;
    
                end
                
                % Frequência dominante ao longo do tempo (somente onde a magnitude é válida)
                indValidos = ~isnan(indMax) & indMax > 0 & indMax <= length(freq);
                freqDominante_tempo = zeros(1, length(indMax));
                freqDominante_tempo(indValidos) = freq(indMax(indValidos));
        
                %Calcular a frequência do sinal sem ruído
                freqValidos = freqDominante_tempo(freqDominante_tempo > 0);
        
                if isempty(freqValidos)
                    fDominante = 0;
                else
                    fDominante = mean(freqValidos);
                end
        
                fprintf('\n')
                k = 0;
    
                % Indicar a frequência do sinal
                if fDominante == 0
                    fprintf('Não existe onda\n')
    
                else
                    for a = 0:3:12
    
                        k = k+1;
                        if fDominante/(10^a) <1
                            fprintf('Frequência média ao longo do tempo: %.4f %.3s\n', fDominante/(10^(a-3)), escala{k-1});
                            break
    
                        end
                    end
                end
                
                % Gráfico wavelets
                figure;
                cwt(volts,Fs);
                title('Análise por wavelets')
                
                % Verificar se é relâmpago, distúrbio ou ruído
                if pkV>watchdogThreshold & fDominante > 10e3 & fDominante < 10e4
    
                    if contains(lower(linha),'relâmpago')
                        fprintf('O sinal verificado é possivelmente um relâmpago\n')
                    else
                        fprintf('O sensor disse "%s" e o programa diz que é relâmpago\n',linha)
                    end
                
                elseif pkV>watchdogThreshold & (fDominante < 10e3 | fDominante > 10e4)
    
                    if contains(lower(linha),'disturbio')
                        fprintf('O sinal verificado é apenas um distúrbio\n')
                    else
                        fprintf('O sensor disse "%s" e o programa diz que é distúrbio\n',linha)
                    end
                
                else 
    
                    if contains(lower(linha),'ruído')
                        fprintf('O sinal verificado é apenas ruído\n')
                    else
                        fprintf('O sensor disse "%s" e o programa diz que é ruído\n',linha)
                    end
                end
                
                % Gráfico da frequência ao longo do tempo
                figure;
                plot(t * 1e9, freqDominante_tempo / 1e6, 'LineWidth', 1.2), grid on
                title('Frequência ao longo do tempo');
                xlabel('Tempo (ns)');
                ylabel('Frequência (MHz)');
                xlim([min(t)*1e9, max(t)*1e9])
                
                figure;
                
                % Gráfico da frequência ao longo do tempo mais atenuado
                freq_suave = movmean(freqDominante_tempo, 15);
                plot(t*1e9, freq_suave / 1e6, 'LineWidth',1.2), grid on
                title('Frequência ao longo do tempo (atenuado)');
                xlabel('Tempo (ns)');
                ylabel('Frequência (MHz)');
                xlim([min(t)*1e9, max(t)*1e9])
    
                t = t';
                volts = volts';
                T = table(t,volts);
                %W = table(cwtCoeffs, freq);
    
                if contains(lower(linha),'disturbio')
                    contadorD = contadorD+1;
                    nome = ['Disturbio_' num2str(contadorD)];
                    writetable(T, 'Sinais.xlsx', 'Sheet', nome);
                    %writetable(W, 'Wavelets.xlsx', 'Sheet', nome);
                end
    
                if contains(lower(linha),'relampago')
                    contadorR = contadorR+1;
                    nome = ['Relampago_' num2str(contadorR)];
                    writetable(T, 'Sinais.xlsx', 'Sheet', nome);
                    %writetable(W, 'Wavelets.xlsx', 'Sheet', nome);
                end
                
                close all
    
                writeline(arduinoSerial, "WAIT");
                pause(0.2);
                writeline(dev, ":SINGLE");
                pause(0.2);
            catch ME
                if contains(ME.message, 'timeout expired')
                    writeline(arduinoSerial, "WAIT");
                    pause(0.2);
                    writeline(dev, ":SINGLE");
                    pause(0.2);
                    continue
                end
            end
        end
    end
end

clear arduinoSerial