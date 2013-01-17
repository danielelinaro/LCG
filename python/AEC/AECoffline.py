'''
Active Electrode Compensation (AEC)
Offline electrode kernel estimation and compensation (current-clamp).
R. Brette 2008 (brette@di.ens.fr)

The technique was presented in the following paper:
High-resolution intracellular recordings using a real-time computational model of the electrode
R. Brette, Z. Piwkowska, C. Monier, M. Rudolph-Lilith, J. Fournier, M. Levy, Y. Fregnac, T. Bal, A. Destexhe

'''
from scipy import convolve,optimize,zeros,mean,exp,array,arange,diff,Inf

def AEC_compensate(v,i,ke):
    '''
    Active Electrode Compensation, done offline.
    v = recorded potential
    i = injected current
    ke = electrode kernel
    Returns the compensated potential.
    '''
    return v-convolve(ke,i)[:-(len(ke)-1)]

def full_kernel(v,i,ksize,full_output=False):
    '''
    Calculates the full kernel from the recording v and the input
    current i. The last ksize steps of v should be null.
    ksize = size of the resulting kernel
    full_output = returns K,v0 if True (v0 is the resting potential)
    '''
    # Calculate the correlation vector <v(n)i(n-k)>
    # and the autocorrelation vector <i(n)i(n-k)>
    vi=zeros(ksize)
    ii=zeros(ksize)
    vref=mean(v) # taking <v> as the reference potential simplifies the formulas
    for k in range(ksize):
        vi[k]=mean((v[k:]-vref)*i[:len(i)-k])
        ii[k]=mean(i[k:]*i[:len(i)-k])
    vi-=mean(i)**2
    K=levinson_durbin(ii,vi)
    if full_output:
        v0=vref-mean(i)*sum(K)
        return K,v0
    else:
        return K

def full_kernel_from_step(V,I):
    '''
    Calculates the full kernel from the response (V) to a step input
    (I, constant).
    '''
    return diff(V)/I

def electrode_kernel(Karg,start_tail,full_output=False):
    '''
    Extracts the electrode kernel Ke from the raw kernel K
    by removing the membrane kernel, estimated from the
    indexes >= start_tail of the raw kernel.
    full_output = returns Ke,Km if True (otherwise Ke)
    (Ke=electrode filter, Km=membrane filter)
    '''

    K=Karg.copy()

    def remove_km(RawK,x,tau,tail):
        '''
        Solves Ke = RawK - Km * Ke/Re for an exponential Km.
        Returns the error on the tail (index vector).
        '''
    
        alpha=x/tau
        lambd=exp(-1./tau)
        Y=zeros(len(RawK))
        Y[0]=alpha/(alpha+1.)*RawK[0]
        for i in range(1,len(RawK)):
            Y[i]=(alpha*RawK[i]+lambd*Y[i-1])/(1+alpha)
        Kel=RawK-Y
        err=sum(Kel[tail]**2)
        return (err,Kel)

    # Exponential fit of the tail to find the membrane time constant
    tail=range(start_tail,len(K))
    result=exp_fit(K[tail])

    # Membrane time constant
    tau = result['tau'] # in units of the bin size
    f=lambda x:result['f'](x)*exp(start_tail/tau)
    #print tau =',tau

    # Replace tail by fit (reduces noise)
    K[tail]=f(array(tail))
    
    # Find first local minimum
    # Gross estimate
    zmin=Inf
    r=1.1
    x0=.1
    for _ in range(70):
        z=remove_km(K,x0,tau,tail)[0]
        if z>zmin: # stop here
            break
        zmin=z
        x0=x0*r # logarithmic steps
    xmin=x0
    
    # Refine the minimum
    z=optimize.fminbound(lambda x:remove_km(K,x,tau,tail)[0],xmin/2.,xmin*2.)
    _,Ke=remove_km(K,z,tau,tail)

    if full_output:
        return Ke[:start_tail],f(arange(len(K)))
    else:
        return Ke[:start_tail]

def levinson_durbin(a,y):
    '''
    Solves AX=Y where A is a symetrical Toeplitz matrix with coefficients
    given by the vector a (a = first row = first column of A).
    '''
    b=0*a
    x=0*a
    b[0]=1./a[0]
    x[0]=y[0]*b[0]
    for i in range(1,len(a)):
        alpha=sum(a[1:i+1]*b[:i])
        u=1./(1-alpha**2)
        v=-alpha*u
        tmp=b[i-1]
        if i>1:
            b[1:i]=v*b[i-2::-1]+u*b[:i-1]
        b[0]=v*tmp
        b[i]=u*tmp
        beta=y[i]-sum(a[i:0:-1]*x[:i])
        x+=beta*b
    return x

def exp_fit(K):
    '''
    Fits vector K to an exponential function (least square).
        K = R/tau*exp(-t/tau)
    where t=0..n-1
    Returns a dictionary with keys R, tau and f (the fit function).
    K = vector of values
    '''
    t=arange(len(K))
    f=lambda params:params[0]*exp(-params[1]**2*t)-K
    p,_=optimize.leastsq(f,array([1.,.3]))
    tau=1./(p[1]**2)
    R=p[0]*tau
    f=lambda s:(R/tau)*exp(-s/tau)
    return {'tau':tau,
            'R':R,
            'f':f}
