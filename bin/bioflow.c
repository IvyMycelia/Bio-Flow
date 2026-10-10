#include <stdio.h>
typedef struct { char* data; int length; } flower_string;
static int flower_cstr_len(char* s) {
  int i = 0;
  if (s == NULL) return 0;
  while (s[i] != '\0') i++;
  return i;
}
static flower_string flower_string_from_cstr(char* s) {
  flower_string out;
  out.data = s;
  out.length = (int)flower_cstr_len(s);
  return out;
}
static int flower_string_eq(flower_string a, flower_string b) {
  int i = 0;
  if (a.length != b.length) return 0;
  while (i < a.length) {
      if (a.data[i] != b.data[i]) return 0;
      i++;
  }
  return 1;
}
static void flower_print_string(flower_string s) {
  int i = 0;
  while (i < s.length) {
      putchar(s.data[i]);
      i++;
  }
}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


typedef struct Species {
int row;
char* id;
int size;
int missing_size;
int counts[5];
} Species;


typedef struct Analysis {
Species* species;
int species_processed;
int active_species;
char* protein_id;
int claims[5];
int supported;
int group_distinct;
int group_duplicates;
int metadata_rows;
int annotation_rows;
int duplicate_metadata_rows;
int duplicate_annotation_rows;
int rejected_metadata_rows;
int rejected_annotation_rows;
int missing_fields;
int invalid_numeric_fields;
int unique_proteins;
int classified_proteins;
int ambiguous_classifications;
int conflicting_classifications;
int normalization_failures;
int normalization_failed_cells;
int distinct_annotation_records;
int accepted_annotation_records;
} Analysis;


int mod_src_classify_flo_equal(char* left, char* right) {
int comparison = strcmp(left, right);
return comparison == 0;
}


char* mod_src_classify_flo_family(int index) {
if (index == 0) {
return (((flower_string){ "laccase_associated", (int)(sizeof("laccase_associated") - 1) })).data;
}
else if (index == 1) {
return (((flower_string){ "manganese_peroxidase", (int)(sizeof("manganese_peroxidase") - 1) })).data;
}
else if (index == 2) {
return (((flower_string){ "lignin_peroxidase", (int)(sizeof("lignin_peroxidase") - 1) })).data;
}
else if (index == 3) {
return (((flower_string){ "versatile_peroxidase", (int)(sizeof("versatile_peroxidase") - 1) })).data;
}
return (((flower_string){ "cytochrome_p450", (int)(sizeof("cytochrome_p450") - 1) })).data;
}


int mod_src_classify_flo_family_id(char* name) {
for (int i = 0; i < 5; i++) {
if (mod_src_classify_flo_equal(name, mod_src_classify_flo_family(i))) {
return i;
}
}
return -1;
}


int mod_src_classify_flo_evidence_supported(char* name) {
int curated = mod_src_classify_flo_equal(name, (((flower_string){ "family_specific_curated_homology", (int)(sizeof("family_specific_curated_homology") - 1) })).data);
int experimental = mod_src_classify_flo_equal(name, (((flower_string){ "family_specific_experimental", (int)(sizeof("family_specific_experimental") - 1) })).data);
return curated  ||  experimental;
}


int mod_src_classify_flo_evidence_valid(char* name) {
if (mod_src_classify_flo_evidence_supported(name)) {
return 1;
}
int domain = mod_src_classify_flo_equal(name, (((flower_string){ "domain_only", (int)(sizeof("domain_only") - 1) })).data);
int keyword = mod_src_classify_flo_equal(name, (((flower_string){ "keyword_only", (int)(sizeof("keyword_only") - 1) })).data);
return domain  ||  keyword;
}


int mod_src_classify_flo_confidence_valid(char* name) {
int high = mod_src_classify_flo_equal(name, (((flower_string){ "high", (int)(sizeof("high") - 1) })).data);
int medium = mod_src_classify_flo_equal(name, (((flower_string){ "medium", (int)(sizeof("medium") - 1) })).data);
int low = mod_src_classify_flo_equal(name, (((flower_string){ "low", (int)(sizeof("low") - 1) })).data);
return high  ||  medium  ||  low;
}


void mod_src_classify_flo_begin_group(Analysis* q, int species_index, char* protein) {
q->active_species = species_index;
q->protein_id = protein;
q->supported = 0;
q->group_distinct = 0;
q->group_duplicates = 0;
for (int i = 0; i < 5; i++) {
q->claims[i] = 0;
}
}


void mod_src_classify_flo_finish_group(Analysis* q) {
if (q->active_species < 0) {
return;}
int number_of_families = 0;
int selected = -1;
for (int i = 0; i < 5; i++) {
if (q->claims[i] == 1) {
number_of_families = number_of_families + 1;
selected = i;
}
}
char* status = "ambiguous";
char* assigned = "";
Species* species = q->species + q->active_species;
q->unique_proteins = q->unique_proteins + 1;
if (number_of_families > 1) {
status = "conflicting";
q->conflicting_classifications = q->conflicting_classifications + 1;
}
else if (q->supported) {
status = "classified";
assigned = mod_src_classify_flo_family(selected);
species->counts[selected] = species->counts[selected] + 1;
q->classified_proteins = q->classified_proteins + 1;
}
else {
q->ambiguous_classifications = q->ambiguous_classifications + 1;
}
FILE* out = bf_out(2);
fprintf(out, "%s\t%s\t%s\t%s\t", species->id, q->protein_id, status, assigned);
char* separator = "";
for (int i = 0; i < 5; i++) {
if (q->claims[i] == 1) {
fprintf(out, "%s%s", separator, mod_src_classify_flo_family(i));
separator = ";";
}
}
fprintf(out, "\t%d\t%d\n", q->group_distinct, q->group_duplicates);
}


double mod_src_classify_flo_normalize(int count, int proteome_size) {
double numerator = (double)(count);
numerator = 1000.0 * numerator;
double denominator = (double)(proteome_size);
return numerator / denominator;
}


void mod_src_main_flo_write_headers() {
FILE* diagnostics = bf_out(0);
FILE* evidence = bf_out(1);
FILE* assignment = bf_out(2);
FILE* counts = bf_out(3);
FILE* features = bf_out(4);
FILE* qc = bf_out(5);
fprintf(diagnostics, "input_file\tinput_row\treason\trecord_json\n");
fprintf(evidence, "species_id\tprotein_id\tfunctional_family\tannotation_source\tevidence_type\tconfidence_level\tinput_row\tduplicate\trecord_json\n");
fprintf(assignment, "species_id\tprotein_id\tstatus\tassigned_family\tcandidate_families\tdistinct_evidence_rows\tduplicate_evidence_rows\n");
fprintf(qc, "key\tvalue\n");
char* metadata = "species_id\tscientific_name\tstrain_id\tassembly_accession\tproteome_size\tassembly_completeness\tdata_source";
fprintf(counts, "%s", metadata);
fprintf(features, "%s", metadata);
for (int i = 0; i < 5; i++) {
fprintf(counts, "\t%s_count", mod_src_classify_flo_family(i));
fprintf(features, "\t%s_count", mod_src_classify_flo_family(i));
}
for (int i = 0; i < 5; i++) {
fprintf(features, "\t%s_per_1000", mod_src_classify_flo_family(i));
}
fprintf(counts, "\n");
fprintf(features, "\tnormalization_status\n");
}


int mod_src_main_flo_find_species(Analysis* q, char* id) {
int low = 0;
int high = q->species_processed;
while (low < high) {
int distance = high - low;
int middle = low + distance / 2;
Species* candidate = q->species + middle;
int comparison = strcmp(id, candidate->id);
if (comparison == 0) {
return middle;
}
else if (comparison < 0) {
high = middle;
}
else {
low = middle + 1;
}
}
return -1;
}


int mod_src_main_flo_read_metadata(Analysis* q, int* columns) {
int header_width = bf_width(0, 0);
q->species = malloc(sizeof(Species) * (q->metadata_rows));
if (q->species == NULL) {
printf("Unable to allocate species records\n");
return 2;
}
for (int offset = 0; ((0) > (q->metadata_rows)) ? offset > (q->metadata_rows) : offset < (q->metadata_rows); ((0) > (q->metadata_rows)) ? offset-- : offset++) {
int row = offset + 1;
int width = bf_width(0, row);
if (width != header_width) {
bf_reject(0, row, (((flower_string){ "wrong_column_count", (int)(sizeof("wrong_column_count") - 1) })).data);
q->rejected_metadata_rows = q->rejected_metadata_rows + 1;
continue;
}
int missing_required = 0;
for (int i = 0; i < 7; i++) {
char* field = bf_get(0, row, columns[i]);
if (field[0] == '\0') {
q->missing_fields = q->missing_fields + 1;
if (i != 4  &&  i != 5) {
missing_required = 1;
}
}
}
char* size_text = bf_get(0, row, columns[4]);
char* completeness_text = bf_get(0, row, columns[5]);
int size = 0;
double completeness = 0.0;
int size_status = bf_integer(size_text, &(size));
int completeness_status = bf_decimal(completeness_text, &(completeness));
int outside = completeness < 0.0  ||  completeness > 100.0;
if (completeness_status == 1  &&  outside) {
completeness_status = -1;
}
if (size_status < 0) {
q->invalid_numeric_fields = q->invalid_numeric_fields + 1;
}
if (completeness_status < 0) {
q->invalid_numeric_fields = q->invalid_numeric_fields + 1;
}
if (missing_required) {
bf_reject(0, row, (((flower_string){ "missing_required_fields", (int)(sizeof("missing_required_fields") - 1) })).data);
q->rejected_metadata_rows = q->rejected_metadata_rows + 1;
continue;
}
if (size_status < 0  ||  completeness_status < 0) {
bf_reject(0, row, (((flower_string){ "invalid_numeric_fields", (int)(sizeof("invalid_numeric_fields") - 1) })).data);
q->rejected_metadata_rows = q->rejected_metadata_rows + 1;
continue;
}
char* id = bf_get(0, row, columns[0]);
if (q->species_processed > 0) {
int previous_index = q->species_processed - 1;
Species* previous = q->species + previous_index;
if (mod_src_classify_flo_equal(id, previous->id)) {
int identical = bf_same(0, previous->row, row);
if (identical == 1) {
q->duplicate_metadata_rows = q->duplicate_metadata_rows + 1;
continue;
}
bf_reject(0, row, (((flower_string){ "conflicting_species_metadata", (int)(sizeof("conflicting_species_metadata") - 1) })).data);
q->rejected_metadata_rows = q->rejected_metadata_rows + 1;
return 2;
}
}
Species* species = q->species + q->species_processed;
species->row = row;
species->id = id;
species->size = size;
species->missing_size = size_status == 0;
for (int i = 0; i < 5; i++) {
species->counts[i] = 0;
}
q->species_processed = q->species_processed + 1;
}
if (q->species_processed == 0) {
flower_print_string(((flower_string){ "No valid species metadata records\n", (int)(sizeof("No valid species metadata records\n") - 1) }));
return 2;
}
return 0;
}


void mod_src_main_flo_read_annotations(Analysis* q, int* columns) {
int header_width = bf_width(1, 0);
int previous_valid = -1;
FILE* evidence_out = bf_out(1);
for (int offset = 0; ((0) > (q->annotation_rows)) ? offset > (q->annotation_rows) : offset < (q->annotation_rows); ((0) > (q->annotation_rows)) ? offset-- : offset++) {
int row = offset + 1;
int width = bf_width(1, row);
if (width != header_width) {
bf_reject(1, row, (((flower_string){ "wrong_column_count", (int)(sizeof("wrong_column_count") - 1) })).data);
q->rejected_annotation_rows = q->rejected_annotation_rows + 1;
continue;
}
int missing = 0;
for (int i = 0; i < 6; i++) {
char* field = bf_get(1, row, columns[i]);
if (field[0] == '\0') {
q->missing_fields = q->missing_fields + 1;
missing = 1;
}
}
char* species_id = bf_get(1, row, columns[0]);
char* protein_id = bf_get(1, row, columns[1]);
char* family_name = bf_get(1, row, columns[2]);
char* evidence_type = bf_get(1, row, columns[4]);
char* confidence = bf_get(1, row, columns[5]);
int species_index = mod_src_main_flo_find_species(q, species_id);
int family_index = mod_src_classify_flo_family_id(family_name);
char* reason = "";
if (missing) {
reason = "missing_required_fields";
}
else if (species_index < 0) {
reason = "unknown_species_id";
}
else if (family_index < 0) {
reason = "unknown_functional_family";
}
else if (!(mod_src_classify_flo_evidence_valid(evidence_type))) {
reason = "unknown_evidence_type";
}
else if (!(mod_src_classify_flo_confidence_valid(confidence))) {
reason = "unknown_confidence_level";
}
if (reason[0] != '\0') {
bf_reject(1, row, reason);
q->rejected_annotation_rows = q->rejected_annotation_rows + 1;
continue;
}
q->accepted_annotation_records = q->accepted_annotation_records + 1;
int changed = 1;
if (q->active_species == species_index) {
changed = !(mod_src_classify_flo_equal(q->protein_id, protein_id));
}
if (changed) {
mod_src_classify_flo_finish_group(q);
mod_src_classify_flo_begin_group(q, species_index, protein_id);
}
int duplicate = 0;
if (previous_valid >= 0) {
duplicate = bf_same(1, previous_valid, row);
}
if (duplicate == 1) {
q->duplicate_annotation_rows = q->duplicate_annotation_rows + 1;
q->group_duplicates = q->group_duplicates + 1;
}
else {
previous_valid = row;
q->distinct_annotation_records = q->distinct_annotation_records + 1;
q->group_distinct = q->group_distinct + 1;
q->claims[family_index] = 1;
int supported = mod_src_classify_flo_evidence_supported(evidence_type);
q->supported = q->supported  ||  supported;
}
bf_fields(evidence_out, 1, row, columns, 6);
int original_line = bf_line(1, row);
fprintf(evidence_out, "\t%d\t%d\t", original_line, duplicate);
bf_record_json(evidence_out, 1, row);
fprintf(evidence_out, "\n");
}
mod_src_classify_flo_finish_group(q);
}


void mod_src_main_flo_write_abundance(Analysis* q, int* metadata_columns) {
FILE* counts = bf_out(3);
FILE* features = bf_out(4);
for (int i = 0; ((0) > (q->species_processed)) ? i > (q->species_processed) : i < (q->species_processed); ((0) > (q->species_processed)) ? i-- : i++) {
Species* species = q->species + i;
bf_fields(counts, 0, species->row, metadata_columns, 7);
bf_fields(features, 0, species->row, metadata_columns, 7);
int total = 0;
for (int f = 0; f < 5; f++) {
total = total + species->counts[f];
fprintf(counts, "\t%d", species->counts[f]);
fprintf(features, "\t%d", species->counts[f]);
}
fprintf(counts, "\n");
char* status = "ok";
if (species->missing_size) {
status = "missing_proteome_size";
}
else if (species->size == 0) {
status = "zero_proteome_size";
}
else if (total > species->size) {
status = "candidate_count_exceeds_proteome_size";
}
int usable = mod_src_classify_flo_equal(status, (((flower_string){ "ok", (int)(sizeof("ok") - 1) })).data);
if (!(usable)) {
q->normalization_failures = q->normalization_failures + 1;
q->normalization_failed_cells = q->normalization_failed_cells + 5;
}
for (int f = 0; f < 5; f++) {
if (usable) {
double value = mod_src_classify_flo_normalize(species->counts[f], species->size);
fprintf(features, "\t%.17g", value);
}
else {
fprintf(features, "\tNA");
}
}
fprintf(features, "\t%s\n", status);
}
}


void mod_src_main_flo_write_qc(Analysis* q) {
FILE* out = bf_out(5);
fprintf(out, "metadata_rows\t%d\n", q->metadata_rows);
fprintf(out, "annotation_rows\t%d\n", q->annotation_rows);
fprintf(out, "duplicate_metadata_rows\t%d\n", q->duplicate_metadata_rows);
fprintf(out, "duplicate_annotation_rows\t%d\n", q->duplicate_annotation_rows);
fprintf(out, "rejected_metadata_rows\t%d\n", q->rejected_metadata_rows);
fprintf(out, "rejected_annotation_rows\t%d\n", q->rejected_annotation_rows);
fprintf(out, "missing_fields\t%d\n", q->missing_fields);
fprintf(out, "invalid_numeric_fields\t%d\n", q->invalid_numeric_fields);
fprintf(out, "unique_proteins\t%d\n", q->unique_proteins);
fprintf(out, "classified_proteins\t%d\n", q->classified_proteins);
fprintf(out, "ambiguous_classifications\t%d\n", q->ambiguous_classifications);
fprintf(out, "conflicting_classifications\t%d\n", q->conflicting_classifications);
fprintf(out, "normalization_failures\t%d\n", q->normalization_failures);
fprintf(out, "normalization_failed_cells\t%d\n", q->normalization_failed_cells);
fprintf(out, "distinct_annotation_records\t%d\n", q->distinct_annotation_records);
fprintf(out, "accepted_annotation_records\t%d\n", q->accepted_annotation_records);
fprintf(out, "species_processed\t%d\n", q->species_processed);
}


int mod_src_main_flo_run(Analysis* q, char* species_path, char* annotation_path) {
int loaded = bf_load(0, species_path);
if (loaded != 0) {
return 2;
}
q->metadata_rows = bf_rows(0);
loaded = bf_load(1, annotation_path);
if (loaded != 0) {
return 2;
}
q->annotation_rows = bf_rows(1);
int metadata[7];
metadata[0] = bf_column(0, (((flower_string){ "species_id", (int)(sizeof("species_id") - 1) })).data);
metadata[1] = bf_column(0, (((flower_string){ "scientific_name", (int)(sizeof("scientific_name") - 1) })).data);
metadata[2] = bf_column(0, (((flower_string){ "strain_id", (int)(sizeof("strain_id") - 1) })).data);
metadata[3] = bf_column(0, (((flower_string){ "assembly_accession", (int)(sizeof("assembly_accession") - 1) })).data);
metadata[4] = bf_column(0, (((flower_string){ "proteome_size", (int)(sizeof("proteome_size") - 1) })).data);
metadata[5] = bf_column(0, (((flower_string){ "assembly_completeness", (int)(sizeof("assembly_completeness") - 1) })).data);
metadata[6] = bf_column(0, (((flower_string){ "data_source", (int)(sizeof("data_source") - 1) })).data);
int annotations[6];
annotations[0] = bf_column(1, (((flower_string){ "species_id", (int)(sizeof("species_id") - 1) })).data);
annotations[1] = bf_column(1, (((flower_string){ "protein_id", (int)(sizeof("protein_id") - 1) })).data);
annotations[2] = bf_column(1, (((flower_string){ "functional_family", (int)(sizeof("functional_family") - 1) })).data);
annotations[3] = bf_column(1, (((flower_string){ "annotation_source", (int)(sizeof("annotation_source") - 1) })).data);
annotations[4] = bf_column(1, (((flower_string){ "evidence_type", (int)(sizeof("evidence_type") - 1) })).data);
annotations[5] = bf_column(1, (((flower_string){ "confidence_level", (int)(sizeof("confidence_level") - 1) })).data);
for (int i = 0; i < 7; i++) {
if (metadata[i] < 0) {
return 2;
}
}
for (int i = 0; i < 6; i++) {
if (annotations[i] < 0) {
return 2;
}
}
if (q->metadata_rows == 0) {
flower_print_string(((flower_string){ "Species metadata contains no records\n", (int)(sizeof("Species metadata contains no records\n") - 1) }));
return 2;
}
int sorted = bf_sort(0, metadata, 7);
if (sorted != 0) {
return 2;
}
sorted = bf_sort(1, annotations, 6);
if (sorted != 0) {
return 2;
}
int status = mod_src_main_flo_read_metadata(q, metadata);
if (status != 0) {
return status;
}
mod_src_main_flo_read_annotations(q, annotations);
mod_src_main_flo_write_abundance(q, metadata);
return 0;
}


int main(int argc, char* argv[]) {
if (argc != 3) {
flower_print_string(((flower_string){ "Usage: bioflow species_metadata.tsv protein_annotations.tsv\n", (int)(sizeof("Usage: bioflow species_metadata.tsv protein_annotations.tsv\n") - 1) }));
flower_print_string(((flower_string){ "Run inside an empty output directory, or use scripts/run.py\n", (int)(sizeof("Run inside an empty output directory, or use scripts/run.py\n") - 1) }));
return 2;
}
Analysis* q = malloc(sizeof(Analysis));
if (q == NULL) {
return 2;
}
memset(q, 0, sizeof(Analysis));
q->active_species = -1;
int opened = bf_open_outputs();
if (opened != 0) {
bf_finish_outputs();
free(q);
return 2;
}
mod_src_main_flo_write_headers();
int status = mod_src_main_flo_run(q, argv[1], argv[2]);
mod_src_main_flo_write_qc(q);
if (q->species != NULL) {
free(q->species);
}
bf_free_inputs();
free(q);
int closed = bf_finish_outputs();
if (status != 0  ||  closed != 0) {
return 2;
}
return 0;
}
