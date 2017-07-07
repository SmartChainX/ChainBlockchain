+echo "Compiling various other contracts and libraries..." 


 
+( 


 
+cd "$REPO_ROOT"/test/compilationTests/ 


 
+for dir in * 


 
+do 


 
+    if [ "$dir" != "README.md" ] 


 
+    then 


 
+        echo " - $dir" 


 
+        cd "$dir" 


 
+        ../../../build/solc/solc --optimize \ 


 
+            --combined-json abi,asm,ast,bin,bin-runtime,clone-bin,compact-format,devdoc,hashes,interface,metadata,opcodes,srcmap,srcmap-runtime,userdoc \ 


 
+            *.sol */*.sol > /dev/null 2>&1 


 
+        cd .. 


 
+    fi 


 
+done 


 
+) 


 
+ 


 
+echo "Running commandline tests..." 


 
+"$REPO_ROOT/test/cmdlineTests.sh" 


 
+ 
