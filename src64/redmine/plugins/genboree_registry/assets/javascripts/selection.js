$( document ).ready(function() {

  // get cds_start cds_end 
  var cds_start = parseInt($("#coding-start").html());
  var cds_end   = parseInt($("#coding-end").html());

  // define a function to convert coordinates in cdna based
  // Please note that the exact same function is in the AssistedHGVS controller
  // As I can't use that in javascript, I am re-writing it
  // If you make change here it is likeli that you need to make change there too
  function convert_absolute_to_cdna (number, cds_start, cds_end){
    if(number <= cds_start){
      return(number - cds_start - 1)
    };
    if(number > cds_start && number <= cds_end){
      return(number - cds_start)
    };
    if(number > cds_end){
      return("*" + (number - cds_end).toString())
    };
  };
  

  $('div').on('keyup', 'input' ,function(){
      if ($('#alternateAllele').val() != ''){
          $('#register-button').attr('disabled',false);
      }
      else {
        if($('#referenceAllele').val() == ''){
          $('#register-button').attr('disabled',true);
        }
      }
  });

  $('body').on('click', '#register-button' ,function(){
  	//console.log('Generate HGVS clicked');
	referenceSequence =  $('#reference-sequence-id').text().trim();
        refStartPos = $('#startPos').val();
        refEndPos   = $('#endPos').val();
        refSeq      = $('#referenceAllele').val();
        altSeq      = $('#alternateAllele').val();
        console.log((referenceSequence + '-' + refStartPos + '-' + refEndPos + '-'  + refSeq + '-' + altSeq));
	var hgvsExpression = ""

	if ( refSeq.length == 1 ){
	 if( altSeq.length == 1 ){
	   // substitution
	   hgvsExpression = hgvsExpression + referenceSequence+':c.'+refStartPos+refSeq+'>'+altSeq;
	 } 
	 if( altSeq.length == 0 ){
	   // deletion
	   hgvsExpression = hgvsExpression + referenceSequence+':c.'+refStartPos+'del'+refSeq;
	 }
	 if( altSeq.length > 1){
	   // indel
	   hgvsExpression = hgvsExpression + referenceSequence+':c.'+refStartPos+'del'+refSeq+'ins'+altSeq;
	 }
	}
	if ( refSeq.length == 0 ){
	  if ( altSeq.length != 0 ){
	   hgvsExpression = hgvsExpression + referenceSequence+':c.'+refStartPos+'_'+refEndPos+'ins'+altSeq;
	  }
	}
	if ( refSeq.length > 1 ){
	  if ( altSeq.length == 0 ){
	    // deletion
	   hgvsExpression = hgvsExpression + referenceSequence+':c.'+refStartPos+'_'+refEndPos+'del';
	  }
	  if ( altSeq.length != 0 ){
	    // indel
	   hgvsExpression = hgvsExpression + referenceSequence+':c.'+refStartPos+'_'+refEndPos+'del'+'ins'+altSeq;
	  }
	}
     // Redirect to query allele in a new tab
     console.log("../genboree_registry/allele?"+hgvsExpression);
     window.location.href = "../genboree_registry/allele?hgvs="+hgvsExpression;
  });

  $('div').on('mouseup', '#myArea',function() {
      function generateModal(){
      
      }
      if(document.getElementById ("myArea")){
        var inputText = document.getElementById ("myArea");
         var origSeq = inputText.value;
	 // get the position of start of selection
         var start = inputText.selectionStart;
	 // get the position of start of selection
         var end = inputText.selectionEnd;
	 // upsgream sequence from selection
         var upstreamSeq   = origSeq.slice(0,start).replace(/\n/g,'').replace(/ /g,'');
	 // selected sequence
         var referenceSeq  = origSeq.slice(start,end).replace(/\n/g,'').replace(/ /g,'');
	 // downstream sequence after selection
         var downstreamSeq = origSeq.slice(end,inputText.length).replace(/\n/g,'').replace(/ /g,'');
  
	 $('#register-button').attr('disabled',true);
         if(referenceSeq == 0){
          $('#referenceAllele').val('');
          $('#startPos').val( convert_absolute_to_cdna(upstreamSeq.length, cds_start, cds_end) );

	  cdna_end_position = convert_absolute_to_cdna( upstreamSeq.length , cds_start, cds_end ) 

	  if( cdna_end_position == -1 ) {
	     // This is a special case when last nucleotide is of index -1 then the end will be +1 not zero
             $('#endPos').val(cdna_end_position + 2);
	     // if downstream, then should be handled differently
	  } else {
	     if(cdna_end_position.toString().match(/\*/) != undefined){
                //extract the number, increamet, add star and send
		number = parseInt(cdna_end_position.match(/[0-9]+/)[0]) + 1;
                $('#endPos').val("*"+number.toString());
	     }
	     else{
               $('#endPos').val(cdna_end_position + 1);
	     };
	  };
          $('#alternateAllele').val('');
         }
         if(referenceSeq != 0){
          $('#referenceAllele').val(referenceSeq);
          $('#startPos').val( convert_absolute_to_cdna( (upstreamSeq.length + 1), cds_start, cds_end) );
          $('#endPos').val(convert_absolute_to_cdna((upstreamSeq.length + referenceSeq.length) , cds_start, cds_end));
          $('#alternateAllele').val('');
	  $('#register-button').attr('disabled',false)
         }
        }
  });
  
});
