$(document).ready(function(){

var currentLoc = window.location.href; 
var actualUri = currentLoc.split('?')[0];
var afterUri = currentLoc.split('?')[1];
var regexp1 = /skip=\d+/ ;
var regexp2 = /limit=\d+/ ;

actualUri = currentLoc.replace(/skip=\d+/,'').replace(/limit=\d+/,'')
actualUri = actualUri.replace(/\?&+/,'?');
actualUri = actualUri.replace(/&+/,'&');
actualUri = actualUri.replace(/&+$/,'');

var skip;

try {
	skip = afterUri.match(regexp1)[0];
	skip = skip.split('=')[1];
	skip = parseInt(skip);
}

catch(err) {
	skip = 0
};

var limitVar = 100;

try {
	limitVar = afterUri.match(regexp2)[0];
	limitVar = limitVar.split('=')[1];
	limitVar = parseInt(limitVar);
}

catch(err) {

};

if ( (skip - limitVar) != Math.abs(skip - limitVar) ) {
	$('.previous').addClass('disabled') ;
};

var noRecords = $('#hit-count').html();

noRecords = parseInt(noRecords);

if(isNaN(noRecords)) {
	$('.previous').addClass('disabled') ;
	$('.next').addClass('disabled') ;
}

else if ( ( noRecords - limitVar ) != Math.abs( noRecords - limitVar ) ) {
	$('.next').addClass('disabled') ;

};

display_text = "Showing alleles " + (skip + 1) + " - " + (skip + noRecords) + " from query results"

$("h2:contains('Number of alleles fetched')" ).html(display_text);

$('#next-page').attr('href', actualUri + '&' + 'skip=' + (skip + limitVar) + '&' + 'limit=' + limitVar);
$('#previous-page').attr('href', actualUri +'&' + 'skip=' + (skip - limitVar) + '&' + 'limit=' + limitVar);
});
