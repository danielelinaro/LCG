#!/bin/bash

_lcg()
{
    local cur prev subcommands
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    subcommands="annotate ap correlations cv experiment extracellular fclamp fi fopt help kernel ltpltd non-rt ou output pspopt pulse pulses ramp rate-steps sinusoids steps tau test_daq vcclamp vi zero"
    if [[ ${prev} == "lcg" ]] ; then
        COMPREPLY=( $(compgen -W "${subcommands}" -- ${cur}) )
        return 0
    fi
}

complete -F _lcg lcg

