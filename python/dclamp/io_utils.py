__all__ = ['load']

def load(filename):
    fid = tbl.openFile(filename,mode='r')
    try:
        version = fid.root.Info._v_attrs.version
    except:
        if fid.root.__contains__('Metadata'):
            version = 0
        else:
            version = 1
    fid.close()
    print('H5 file is saved according to version #%d.' % version)
    if version == 0:
        return loadH5TraceV0(filename)
    if version == 1:
        return loadH5TraceV1(filename)
    if version == 2:
        return loadH5TraceV2(filename)
    print('Unknown H5 file version (%d).' % version)

def loadH5TraceV0(filename):
    fid = tbl.openFile(filename,mode='r')
    entities = []
    map = {}
    for k,node in enumerate(fid.root.Data):
        id = int(node.name.split('-')[1])
        map[id] = k
        entities.append({'data': node.read(), 'id': id})
    for node in fid.root.Metadata:
        id = int(node.name.split('-')[1])
        entities[map[id]]['metadata'] = node.read()
    for node in fid.root.Parameters:
        id = int(node.name.split('-')[1])
        entities[map[id]]['parameters'] = node.read()
    info = {'dt': fid.root.Misc.Simulation_properties.attrs.dt,
            'tend': fid.root.Misc.Simulation_properties.attrs.tend,
            'version': 0}
    fid.close()
    return entities,info

def loadH5TraceV1(filename):
    fid = tbl.openFile(filename,mode='r')
    entities = []
    for node in fid.root.Data:
        id = int(node.name.split('-')[1])
        entities.append({
                'id': id,
                'data': node.read()})
        for attrName in node.attrs._v_attrnames:
            if len(attrName) > 8 and attrName[:8].lower() == 'metadata':
                entities[-1]['metadata'] = node.attrs[attrName]
            else:
                entities[-1][attrName.lower()] = node.attrs[attrName]
    info = {'dt': fid.root.Misc.Simulation_properties.attrs.dt,
            'tend': fid.root.Misc.Simulation_properties.attrs.tend,
            'version': 1}
    fid.close()
    return entities,info

def loadH5TraceV2(filename):
    fid = tbl.openFile(filename,mode='r')
    try:
        if fid.root.Info._v_attrs.version != 2:
            print('Version not supported: %d.' % version)
            return
    except:
        print('Unknown version.')
        return

    entities = []
    for node in fid.root.Entities:
        entities.append({
                'id': node._v_name,
                'data': node.Data.read()})
        try:
            entities[-1]['metadata'] = node.Metadata.read()
        except:
            pass
        for attrName in node._v_attrs._v_attrnames:
            entities[-1][attrName.lower()] = node._v_attrs[attrName]
    info = {'dt': fid.root.Info._v_attrs.dt,
            'tend': fid.root.Info._v_attrs.tend,
            'version': fid.root.Info._v_attrs.version}
    fid.close()
    return entities,info

