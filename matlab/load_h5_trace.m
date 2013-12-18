function [entities,info] = load_h5_trace(filename)
% [entities,info] = loadH5Trace(filename)
%
% Loads an HDF5 file recorded with lcg.
%

warning off
try
    version = hdf5read(filename, '/Info/version');
catch
    version = 1;
    info = hdf5info(filename);
    ngroups = length(info.GroupHierarchy.Groups);
    for k=1:ngroups
        if strcmp(info.GroupHierarchy.Groups(k).Name, '/Metadata');
            version = 0;
            break;
        end
    end
end

%fprintf(1, 'H5 file version #%.0f.\n', version);
% Support for deprecated versions.
switch version
    case 0
        [entities,info] = loadV0(filename);
    case 1
        [entities,info] = loadV1(filename);
    case 2
        [entities,info] = loadV2(filename);
    otherwise
        error('Unknown H5 file version (%.0f).', version);
end
warning on


function [entities,info] = loadV0(filename)
% out = loadH5TraceV0(filename)

info = hdf5info(filename);
ngroups = length(info.GroupHierarchy.Groups);

for ii=1:ngroups
    if strcmp(info.GroupHierarchy.Groups(ii).Name,'/Data') == 1
        nentities = length(info.GroupHierarchy.Groups(ii).Datasets);
        entities = repmat(struct('id',[],'data',[],'metadata',[],'parameters',[]), [nentities,1]);
        ids = zeros(nentities,1);
        for jj=1:nentities
            ids(jj) = str2double(info.GroupHierarchy.Groups(ii).Datasets(jj).Name(end-3:end));
            entities(jj).id = ids(jj);
            data = hdf5read(filename, info.GroupHierarchy.Groups(ii).Datasets(jj).Name);
            if iscolumn(data) && min(size(data))==1
                data = data';
            end
            entities(jj).data = data;
        end
    elseif strcmp(info.GroupHierarchy.Groups(ii).Name,'/Metadata') == 1
        for jj=1:length(info.GroupHierarchy.Groups(ii).Datasets)
            name = info.GroupHierarchy.Groups(ii).Datasets(jj).Name;
            idx = strfind(name, '-');
            ntt = find(ids == str2double(name(idx(1)+1:idx(2)-1)));
            entities(ntt).metadata = hdf5read(filename, name)';
        end
    elseif strcmp(info.GroupHierarchy.Groups(ii).Name,'/Parameters') == 1
        for jj=1:length(info.GroupHierarchy.Groups(ii).Datasets)
            name = info.GroupHierarchy.Groups(ii).Datasets(jj).Name;
            ntt = find(ids == str2double(name(end-3:end)));
            entities(ntt).parameters = hdf5read(filename, name);
        end
    end
end

clear('info');
info.tend = hdf5read(filename,'/Misc/Simulation_properties/tend');
info.dt = hdf5read(filename,'/Misc/Simulation_properties/dt');
info.srate = 1.0 / info.dt;
info.version = 0;


function [entities,info] = loadV1(filename)
% [entities,info] = loadH5TraceV1(filename)

info = hdf5info(filename);
ngroups = length(info.GroupHierarchy.Groups);

for ii=1:ngroups
    if strcmp(info.GroupHierarchy.Groups(ii).Name,'/Data') == 1
        nentities = length(info.GroupHierarchy.Groups(ii).Datasets);
        entities = repmat( ...
            struct('id',[],'data',[],'metadata',[],'units','','name',''), ...
            [nentities,1]);
        for jj=1:nentities
            name = info.GroupHierarchy.Groups(ii).Datasets(jj).Name;
            entities(jj).id = str2double(name(end-3:end));
            data = hdf5read(filename, name);
            if iscolumn(data) && min(size(data))==1
                data = data';
            end
            entities(jj).data = data;
            nattrs = length(info.GroupHierarchy.Groups(ii).Datasets(jj).Attributes);
            for kk=1:nattrs
                name = info.GroupHierarchy.Groups(ii).Datasets(jj).Attributes(kk).Name;
                idx = strfind(name, '/');
                name = lower(name(idx(end)+1:end));
                if length(name) > 8 && strcmp(name(1:8),'metadata')
                    entities(jj).metadata = ...
                        info.GroupHierarchy.Groups(ii).Datasets(jj).Attributes(kk).Value';
                else
                    value = info.GroupHierarchy.Groups(ii).Datasets(jj).Attributes(kk).Value;
                    if isa(value, 'hdf5.h5string')
                        entities(jj).(name) = value.Data;
                    else
                        entities(jj).(name) = value;
                    end
                end
            end
        end
    end
end

clear('info');
info.tend = hdf5read(filename,'/Misc/Simulation_properties/tend');
info.dt = hdf5read(filename,'/Misc/Simulation_properties/dt');
info.srate = 1.0 / info.dt;
info.version = 1;

function [entities,info] = loadV2(filename)
% [entities,info] = loadH5TraceV2(filename)

try
    version = hdf5read(filename, '/Info/version');
    if version ~= 2
        error('Unknown version in H5 file.');
    end
catch
    error('Unknown version in H5 file.');
end

info = hdf5info(filename);
ngroups = length(info.GroupHierarchy.Groups);

for ii=1:ngroups
    if strcmp(info.GroupHierarchy.Groups(ii).Name,'/Entities') == 1
        nentities = length(info.GroupHierarchy.Groups(ii).Groups);
        entities = repmat( ...
            struct('id',[],'data',[],'metadata',[],'units','','name','','parameters',struct([])), ...
            [nentities,1]);
        for jj=1:nentities
            name = info.GroupHierarchy.Groups(ii).Groups(jj).Name;
            idx = strfind(name, '/');
            entities(jj).id = str2double(name(idx(end)+1:end));
            data = hdf5read(filename, [name,'/Data']);
            
            if iscolumn(data) && min(size(data))==1
                data = data';
            end
            entities(jj).data = data;
            try
                entities(jj).metadata = hdf5read(filename, [name,'/Metadata'])';
            catch
            end
            nattrs = length(info.GroupHierarchy.Groups(ii).Groups(jj).Attributes);
            for kk=1:nattrs
                name = info.GroupHierarchy.Groups(ii).Groups(jj).Attributes(kk).Name;
                idx = strfind(name, '/');
                name = lower(name(idx(end)+1:end));
                value = info.GroupHierarchy.Groups(ii).Groups(jj).Attributes(kk).Value;
                if isa(value, 'hdf5.h5string')
                    value = value.Data;
                end
                entities(jj).(name) = value;
            end
            nsubgroups = length(info.GroupHierarchy.Groups(ii).Groups(jj).Groups);
            for kk=1:nsubgroups
                name = info.GroupHierarchy.Groups(ii).Groups(jj).Groups(kk).Name;
                idx = strfind(name, '/');
                name = name(idx(end)+1:end);
                if strcmpi(name, 'parameters')
                    npars = length(info.GroupHierarchy.Groups(ii).Groups(jj).Groups(kk).Attributes);
                    for ll=1:npars
                        name = info.GroupHierarchy.Groups(ii).Groups(jj).Groups(kk).Attributes(ll).Name;
                        idx = strfind(name, '/');
                        name = lower(name(idx(end)+1:end));
                        value = info.GroupHierarchy.Groups(ii).Groups(jj).Groups(kk).Attributes(ll).Value;
                        if isa(value, 'hdf5.h5string')
                            value = value.Data;
                        end
                        entities(jj).parameters(1).(name) = value;
                    end
                end
            end
        end
    end
end

clear('info');
info.tend = hdf5read(filename,'/Info/tend');
info.dt = hdf5read(filename,'/Info/dt');
info.srate = 1.0 / info.dt;
info.version = 2;
