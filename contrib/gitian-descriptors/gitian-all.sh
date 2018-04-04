
	./bin/gbuild --commit EvolveChain-Core=${VERSION} ../EvolveChain-Core/contrib/gitian-descriptors/gitian-linux.yml
	mkdir build/out
	pushd build/out
	zip -r evolvechain-${VERSION}-linux.zip *
	mv evolvechain-${VERSION}-linux.zip ../../../evolvechain-0.9.3-linux.zip 
	popd

	./bin/gbuild --commit EvolveChain-Core=${VERSION} ../EvolveChain-Core/contrib/gitian-descriptors/gitian-win.yml
	mkdir build/out
	pushd build/out
	zip -r evolvechain-${VERSION}-win-gitian.zip *
	mv evolvechain-${VERSION}-win-gitian.zip ../../../evolvechain-0.9.3-win.zip
	popd
	
 ./bin/gbuild --commit EvolveChain-Core=${VERSION} ../EvolveChain-Core/contrib/gitian-descriptors/gitian-osx-evolvechain.yml
	mkdir build/out
	pushd build/out
	mv EvolveChain-Qt.dmg ../../../
	popd

	pushd /home/debian
	unzip evolvechain-0.9.3-linux.zip -d evolvechain-0.9.3-linux
	tar czvf evolvechain-0.9.3-linux.tar.gz evolvechain-0.9.3-linux
	rm -rf evolvechain-0.9.3-linux
	
	unzip evolvechain-0.9.3-win.zip -d evolvechain-0.9.3-win
	rm evolvechain-0.9.3-win/32/test*.* -rf
	rm evolvechain-0.9.3-win/64/test*.* -rf
	rm evolvechain-0.9.3-win.zip -rf
	zip -r evolvechain-0.9.3-win.zip evolvechain-0.9.3-win
	rm -rf evolvechain-0.9.3-win
 popd
