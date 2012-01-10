clear all;
close all;
clc;

%%
filename = 'autapse.h5';
info = hdf5info(filename);
nDatasets = length(info.GroupHierarchy.Datasets);
data = cell(nDatasets,1);
for k=1:nDatasets
    data{k} = hdf5read(filename, info.GroupHierarchy.Datasets(k).Name);
end
dt = 1/20000;
t = 0 : dt : dt*(length(data{1})-1);

[pks,locs] = findpeaks(data{1},'MinPeakHeight',-50.1);
isi = diff(t(locs));

figure;
for k=1:nDatasets
    subplot(1,nDatasets,k);
    plot(t, data{k}, 'k');
    if k == 1
        hold on;
        plot(t(locs),data{k}(locs),'r.');
    end
end

figure;
plot(isi,'k.');
