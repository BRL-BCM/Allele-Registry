$(document).ready(function(){

var baseURL = ""

$('#start-search').attr('disabled', true);

function openWindow(transferUrl){
   //console.log(transferUrl);
   window.open(transferUrl,"_self");
   //window.open(transferUrl);
}

function getSearchHash() {

  var simpleSearchHash = {}

  //var baseURL = "/redmine/projects/registry/genboree_registry/"

  simpleSearchHash["CAid"] = {
    "url": baseURL + "by_caid?caid=",
    "suffix": "",
    "example": "CA321211"
  }
  simpleSearchHash["CNVid"] = {
    "url": baseURL + "by_cnvid?cnvid=",
    "suffix": "",
    "example": "CNV335619144"
  }
  simpleSearchHash["Canonical Allele Identifier"] = {
    "url": baseURL + "by_canonicalid?canonicalid=",
    "suffix": "",
    "example": "CA321211,CNV326675580"
  }
  simpleSearchHash["HGVS/Descriptor"] = {
    "url": baseURL + "allele?hgvsOrDescriptor=",
    "suffix": "",
    "example": "NM_002496.3:c.64C>T, NCBI36 5q13.2(chr5:?_69767948-70622774_?)x1"
  }
  simpleSearchHash["HGVS"] = {
    "url": baseURL + "allele?hgvs=",
    "suffix": "",
    "example": "NM_002496.3:c.64C>T, ENST00000413465.6:c.637C>T, NC_000017.10:g.7578212G>A"
  }
  simpleSearchHash["HGNC Gene Symbol"] = {
    "url": baseURL + "alleles?gene=",
    "suffix": "&skip=0&limit=100",
    "example": "TP53"
  }
  simpleSearchHash["ClinVar Variation Id"] = {
    "url": baseURL + "alleles?ClinVar.variationId=",
    "suffix": "",
    "example": "214835"
  }
  simpleSearchHash["ClinVar RCV Id"] = {
    "url": baseURL + "alleles?ClinVar.RCV=",
    "suffix": "",
    "example": "RCV000196794"
  }
  simpleSearchHash["dbSNP Id"] = {
    "url": baseURL + "alleles?dbSNP.rs=",
    "suffix": "",
    "example": "369602258"
  }
  simpleSearchHash["ExAC Id"] = {
    "url": baseURL + "alleles?ExAC.id=",
    "suffix": "",
    "example": "5-112043382-A-G"
  }
  simpleSearchHash["gnomAD Id"] = {
    "url": baseURL + "alleles?gnomAD.id=",
    "suffix": "",
    "example": "5-112043382-A-T"
  }
  simpleSearchHash["MyVariant Id (hg19)"] = {
    "url": baseURL + "alleles?MyVariantInfo_hg19.id=",
    "suffix": "",
    "example": "chr9:g.107620835G>A"
  }
  simpleSearchHash["Reference sequence & position"] = {
    "example": "Box1: NM_000546.5; Box2: 1; Box3: 100"
  }
  simpleSearchHash["Any Identifier"] = {
    "url": baseURL + "alleles?name=",
    "suffix": "&skip=0&limit=100",
    "example": "CA321211, RCV000196794"
  }
  simpleSearchHash["PMID"] = {
    "url" : baseURL + "alleles?externalSource=pubmed&p1=",
    "suffix": "&skip=0&limit=100",
    "example" : "23934111"
  }

  return simpleSearchHash
}


function ifIncludes(inValue, inArray) {
  for (i = 0; i < inArray.length; i++) {
    if (inValue == inArray[i]) {
      return (true)
    }
  }
}

function isItSimple(inValue) {
  var simple = ["HGVS", "ClinVar Variation Id", "ClinVar RCV Id", "CAid", "CNVid", "Canonical Allele Identifier", "Any Identifier", "HGVS/Descriptor", "HGNC Gene Symbol", "dbSNP Id", "ExAC Id", "gnomAD Id", "MyVariant Id (hg19)", "PMID"];
  if (ifIncludes(valueSelected, simple)) {
    return (true)
  } else {
    return (false)
  }
}

$(".type-search a").click(function(){
  console.log("Change of value is detected");
  selectedElement = $(this)//document.getElementById("exampleSelect1")
  console.log("This is a selected element")
  console.log(selectedElement)
  console.log("This is the text")
  valueSelected = selectedElement.text()
  console.log(valueSelected);

  if (!isItSimple(valueSelected)) {
    $("#simple-search-form").attr('hidden', true);
    $("#complex-search-form").attr('hidden', false);
    $("#actual-example").html(getSearchHash()[valueSelected]['example'])
  } else {
    $("#simple-search-form").attr('hidden', false);
    $("#complex-search-form").attr('hidden', true);
    $("#actual-example").html(getSearchHash()[valueSelected]['example'])
}});

$("#start-search").click(function(){ 

  typeOfSearch = document.getElementById("exampleSelect1").innerHTML

  if (isItSimple(typeOfSearch)) {
    console.log('Simple Search is selected');
    stringToSearch = document.getElementById("example-text-input").value
    console.log(stringToSearch);
    stringToSearch = stringToSearch.replace(/^\s+|\s+$/gm,'')  // trim
    transferUrl = getSearchHash()[typeOfSearch]["url"] + encodeURIComponent(stringToSearch) + getSearchHash()[typeOfSearch]["suffix"];
    openWindow(transferUrl);
  } else {
    console.log('Complex search is selected');
    identifier = $("#refseq-identifier").val();
    start = $("#refseq-start").val();
    end = $("#refseq-end").val();
    transferUrl = baseURL + "alleles?refseq=" + encodeURIComponent(identifier) + "&begin=" + start + "&end=" + end + "&skip=0&limit=50";
    openWindow(transferUrl);
  }
});

function enableSearch(){
   $('#start-search').attr('disabled', false);
}
function disableSearch() {
   $('#start-search').attr('disabled', true);
}

$('.type-search a').click(function(){
   if($(this).text().match("Select One") == undefined && $('#example-text-input').val() != ""){
      enableSearch();
   } else {
      disableSearch();
   }
});
$('#example-text-input').keyup(function(){
   if($('#exampleSelect1').text().match("Select One") == undefined && $('#example-text-input').val() != ""){
      enableSearch();
   } else {
      disableSearch();
   }
});
$('.complex-search-field').keyup(function(){
   if ( $('#refseq-identifier').val() != "" && 
        $('#refseq-start').val() != "" && 
        $('#refseq-end').val() != "" && 
        $('#exampleSelect1').text().match("Select One") == undefined ) {
      enableSearch();
   } else {
      disableSearch();
   }
});
 //document.getElementById("exampleSelect1").value = "HGVS";
 //myFunction();
});

