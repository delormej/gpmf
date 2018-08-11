files=`find "$search_dir" -name "*.MP4"`

for entry in $("$files")
do
  echo ./gpmfdemo "$entry"  
done

