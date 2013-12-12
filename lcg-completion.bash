#!/bin/bash

_lcg()
{
    local cur prev subcommands
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    subcommands="annotate ap correlations cv experiment extracellular fclamp fi fopt help kernel ltpltd non-rt ou output pspopt pulse pulses pulses-with-level ramp rate-steps rcwrite sinusoids spontaneous steps stimgen tau test-daq vcclamp vi zero"
    case "$prev" in
	"lcg"|"help")
            COMPREPLY=( $(compgen -W "${subcommands}" -- $cur) )
            return 0
	    ;;
        "sinusoids"|"correlations")
	    COMPREPLY=( $(compgen -W "current conductance" -- $cur) )
	    return 0
	    ;;
    esac
    case "${COMP_WORDS[COMP_CWORD-2]}" in
	"experiment"|"non-rt")
	    if [[ $prev == "-c" ]] ; then
		COMPREPLY=( $(compgen -f -X "!*.xml" -- $cur) )
		return 0
	    fi
	    ;;
	"vcclamp"|"non-rt")
	    if [[ $prev == "-f" ]] ; then
		COMPREPLY=( $(compgen -f -X "!*.stim" -- $cur) )
		return 0
	    fi
	    ;;
    esac
}

complete -F _lcg lcg

