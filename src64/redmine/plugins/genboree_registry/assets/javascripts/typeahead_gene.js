$( document ).ready(function() {

  $('#search-ref-seq').click(function(){
      console.log('Clicked perfectly');
  });

  $('#gene-symbol').keyup(function(){
	$('#search-ref-seq').attr('disabled',false);
  });

  // Instantiate the Bloodhound suggestion engine

  var genes = new Bloodhound({
      datumTokenizer: function (datum) {
          return Bloodhound.tokenizers.whitespace(datum.value);
      },
      queryTokenizer: Bloodhound.tokenizers.whitespace,
      remote: {
          url: 'http://rest.genenames.org/search/symbol/'+ '%QUERY' +'*',
          wildcard: '%QUERY',
          filter: function (response) {
              // Map the remote source JSON array to a JavaScript object array
              return $.map(response.response.docs, function (doc) {
                  return {
                      value: doc.symbol
                  };
              });
          }
      }
  });

  // Initialize the Bloodhound suggestion engine
  genes.initialize();
  
  // Instantiate the Typeahead UI
  $('.typeahead').typeahead(null, {
      displayKey: 'value',
      source: genes.ttAdapter()
  });
});
