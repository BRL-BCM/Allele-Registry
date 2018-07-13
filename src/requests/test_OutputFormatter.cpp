#include "OutputFormatter.hpp"
#include <iostream>

int main (int argc, char ** argv)
{
	if (argc != 2) {
		std::cout << "Parameter: value of 'fields' parameter" << std::endl;
		return 1;
	}

	try {

		OutputFormatter doc(documentType::activeGenomicVariant);

		doc.setFormat( responseFormat::prettyJson, argv[1] );

		DocumentActiveGenomicVariant d;

		std::vector<uint32_t> v;
		v.push_back(22);
		v.push_back(33);
		v.push_back(44);

		d.caId.value = 1234567;
		d.identifiers.add( Identifier_dbSNP(234567) );
		d.identifiers.add( Identifier_dbSNP(34567) );
		d.identifiers.add( Identifier_ClinVarVariant(56789,v) );

		// reference databasemust be loaded to do that
//		d.definitionsOnGenomeBuilds.resize(2);
//		d.definitionsOnGenomeBuilds[0].build = ReferenceGenome::rgGRCh38;
//		d.definitionsOnGenomeBuilds[0].chromosome = Chromosome::chr11;
//		d.definitionsOnGenomeBuilds[0].definition.refId.value = 54643;
//		d.definitionsOnGenomeBuilds[0].definition.modifications.resize(1);
//		d.definitionsOnGenomeBuilds[0].definition.modifications[0].newSequence = "SADFS";
//		d.definitionsOnGenomeBuilds[0].definition.modifications[0].originalSequence = "original";
//		d.definitionsOnGenomeBuilds[0].definition.modifications[0].region.setLeftAndLength(33,44);
//		d.definitionsOnGenomeBuilds[1].build = ReferenceGenome::rgNCBI36;
//		d.definitionsOnGenomeBuilds[1].chromosome = Chromosome::chr13;
//		d.definitionsOnGenomeBuilds[1].hgvsDefs = "sfsdfsdf";

		std::cout << doc.createOutput(d) << std::endl;
	} catch (std::exception & e) {
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
