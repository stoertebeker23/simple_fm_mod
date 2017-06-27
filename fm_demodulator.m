clear all
close all 

%% import komplxe baseband signal from C-Programm     
[data,delimiterOut] = importdata('modulated_signal.txt');
data = data(:,1)'./30000;
%% frequencies
pi = 3.1415926;
Fs = 48000; % sampling frequency
Fc = 12000; % center frequency
NFFT=length(data);
Window_Funktion=hann(NFFT);
% Überlappung bei der FFT
% -----------------------
% OVERLAP=0             % keine Überlappung
% OVERLAP=NFFT-1;       % "Totale Überlappung"
% OVERLAP=NFFT/2;       % Überlapp bis zur halben Länge der FFT
OVERLAP=NFFT/2;

%% TP-Filter design
N   = 6;        % FIR filter order 10
Fp  = 12e3;       % 6 kHz passband-edge frequency
Rp  = 0.57565; % Corresponds to 0.01 dB peak-to-peak ripple 0.00057565
Rst = 1e-4;       % Corresponds to 80 dB stopband attenuation
%eqnum = firceqrip(N,fp/(fs/2),[Rp Rst],'passedge'); % eqnum = vec of coeffs
lpFilt = designfilt('lowpassiir','FilterOrder',N, ...
         'PassbandFrequency',Fp,'PassbandRipple',Rp, ...
         'SampleRate',Fs);
%eqnum = firceqrip(N,Fp/(Fs/2),[Rp Rst],'passedge') % eqnum = vec of coeffs
%figure;
%fvtool(eqnum,'Fs',Fs,'Color','White') % Visualize filter

    
% lp_coef=round(eqnum'.*30000);
% fileID = fopen('lp_coef.txt','w');
% fprintf(fileID,'%i\n',lp_coef);
% fclose(fileID);

%%
Y_misch=exp(-2i*pi*Fc/Fs*(1:NFFT)).*data;           % Spektrum um die Mittenfrequenz
                                                % nach unten verschieben
Y_TP0 = filter(lpFilt,Y_misch);    % TP

phase = angle(Y_TP0); %calculate the phase
phase_diff=diff(phase); %diff phase
% unwrap
for i = 1:1:length(phase_diff)
    while abs(phase_diff(1,i)) > pi 
        if phase_diff(1,i) >= 0
            phase_diff(1,i) = phase_diff(1,i) - 2*pi;
        else
            phase_diff(1,i) = phase_diff(1,i) + 2*pi;
        end;
    end;
    phase_diff_unwrap(1,i) = phase_diff(1,i);
end;
output = 0.75.*phase_diff_unwrap(1,:)./pi;
%% Plot
figure;    
    [l f] = psd(Y_misch,NFFT,Fs,Window_Funktion,OVERLAP);
    % Frequenzen symmetrisch um Null
    f = [-flipud(f(2:end)); f];
    l = [flipud(l(2:end)); l];
    subplot(2,1,1)
    plot(f,10*log10(l)), axis tight;
    title('Spektrale Leistungsdichte des Basisbandsignals');     
    grid on;
    xlabel('f in [Hz]');
    ylabel('[dB]')

    [l f] = psd(Y_TP0,NFFT,Fs,Window_Funktion,OVERLAP);
    %% Frequenzen symmetrisch um Null
    f = [-flipud(f(2:end)); f];
    l = [flipud(l(2:end)); l];
    subplot(2,1,2)
    % plot(Window_Funktion(:,1));
    plot(f,10*log10(l)), axis tight;
    title('Spektrale Leistungsdichte des Basisbandsignals nach dem Tiefpass');     
    grid on;
    xlabel('f in [Hz]');
    ylabel('[dB]')

figure(5);  
subplot(2,1,1)
plot(data);
xlim([0 length(data(1,:))-1]);
title('inputbuffer'); 
subplot(2,1,2)
plot(output);
%axis([0 length(output(1,:)) -1 1]);
%xlim([0 length(demod_outputbuffer)-1]);
title('demoduliertes Signal'); 

figure(4);  
subplot(2,1,1)
plot(real(Y_TP0));
title('real'); 
subplot(2,1,2)
plot(imag(Y_TP0));
title('imag'); 
