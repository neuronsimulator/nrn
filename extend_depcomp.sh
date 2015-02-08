if grep -q xlc depcomp ; then
	#echo "depcomp already extended to handle xlc. No change to file."
	true;
else
	sed '/^aix)/,/;;/ {
			H
			s/\.u/.d/
			s/^aix)/xlc) #following fragment is just like AIX but the extension is .d instead of .u/
		}
		/^icc)/ {
			x
			G			
		}
	' depcomp > temp
	mv temp depcomp
fi
