ibtracertm: ibtracert.c ibdiag_common.c ibdiag_common.h ibdiag_version.h config.h
	gcc ibtracert.c ibdiag_common.c -I. -I/usr/include/infiniband -libverbs -losmcomp -libmad -lrt -o ibtracertm
	
clean:
	rm -f ibtracertm