#include "secshare.h"
#include <math.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>

int isPrime(long unsigned int number){
	int prm = 1;
	long unsigned int sqrtn = sqrt(number), i;

	/* interger approx passes it as a prime */
	if(number == 2)
		return 0;

	for (i = 2; prm && (i <= sqrtn); i++){
		if(!(number % i)){
			prm = 0;
		}
	}
	
	return prm;
}

BIGNUM ** get_n_first_primes(long unsigned int n){
	long unsigned int ctr = 0, i;
	BIGNUM ** prms = (BIGNUM **) malloc(sizeof(BIGNUM *)*n);

	for(i = 1; i < ULONG_MAX && ctr < n; i++){
		if(isPrime(i)){
			prms[ctr++] = convert_int2bn(i);
		}
	}

	return prms;
}

BIGNUM * compute_share(BIGNUM ** prms, BIGNUM * v, long unsigned int n, const BIGNUM * s, const BIGNUM * m){	
	long unsigned int i;
	BIGNUM * res = BN_dup(s);
	BIGNUM * exp, * deg, * mult;

	BN_CTX * ctx = BN_CTX_new();
	BN_CTX_start(ctx);
	BN_RECP_CTX * recp = BN_RECP_CTX_new();
	BN_RECP_CTX_set(recp, m, ctx);

	for(i = 1; i < n; i++){
		exp = BN_new();
		mult = BN_new();
		deg = convert_int2bn(i);

		BN_mod_exp(exp, v, deg, m, ctx);
		
		BN_mod_mul_reciprocal(mult, prms[i-1], exp, recp, ctx);
		
		BN_mod_add(res, res, mult, m, ctx);

		BN_free(exp);
		BN_free(mult);
		BN_free(deg);
	}

	BN_CTX_end(ctx);
	BN_CTX_free(ctx);
	BN_RECP_CTX_free(recp);
	
	return res;
}

BIGNUM ** compute_shares(BIGNUM ** prms, BIGNUM ** vs, long unsigned k, long unsigned int n, const BIGNUM * s, const BIGNUM * m){
	BIGNUM ** shrs = (BIGNUM **) malloc(sizeof(BIGNUM *)*k);
	long unsigned int i; 
	
	for(i = 0; i < k; i++){
		shrs[i] = compute_share(prms, vs[i], n, s, m);
	}

	return shrs;
}

BIGNUM * compute_secret(BIGNUM ** shrs, BIGNUM ** vs, long unsigned int n, BIGNUM * m){
	long unsigned int * indexes, i, j;
	BIGNUM ** sub_vs, * s, * nom, * den, * temp_den, * temp_mult, * temp_sum;
	s = BN_new();

	BN_CTX * ctx = BN_CTX_new();
	BN_CTX_start(ctx);
	BN_RECP_CTX * recp = BN_RECP_CTX_new();
	BN_RECP_CTX_set(recp, m, ctx);

	for(i = 0; i < n; i++){
		//temp_den = BN_new();
		temp_sum = BN_new();
		temp_mult = BN_new();

		indexes = serie_excluded_index(n, i);
		sub_vs = arr_mod_subs(vs, vs[i], m, n);

		nom = multiplyer(vs, indexes, m, n-1);

		temp_den = multiplyer(sub_vs, indexes, m, n-1);
		den = BN_mod_inverse(NULL, temp_den, m, ctx);

		BN_mod_mul_reciprocal(temp_mult, nom, den, recp, ctx);
		BN_mod_mul_reciprocal(temp_sum, temp_mult, shrs[i], recp, ctx);

		BN_mod_add(s, s, temp_sum, m, ctx);

		BN_free(temp_den);
		BN_free(temp_sum);
		BN_free(temp_mult);
		BN_free(nom);
		BN_free(den);
		BNs_free(sub_vs, n);
		free(indexes);
	}

	BN_CTX_end(ctx);
	BN_CTX_free(ctx);
	BN_RECP_CTX_free(recp);

	return s;
}

long unsigned int * serie_excluded_index(long unsigned int num_elem, long unsigned int i_ex){
	long unsigned int * serie = (long unsigned int *) malloc(sizeof(long unsigned int)*(num_elem-1));
	long unsigned int i, j;

	for(i = 0, j = 0; j < (num_elem-1); i++){
		if(i != i_ex){
			serie[j++] = i;
		}
	}

	return serie;
}

BIGNUM * multiplyer(BIGNUM ** elems, long unsigned int * indexes, const BIGNUM * m, long unsigned int i_size){
	BIGNUM * res = BN_new();
	BN_one(res);
	BIGNUM * aux;
	long unsigned int i;

	BN_CTX * ctx = BN_CTX_new();
	BN_CTX_start(ctx);
	BN_RECP_CTX * recp = BN_RECP_CTX_new();
	BN_RECP_CTX_set(recp, m, ctx);

	for(i = 0; i < i_size; i++){
		aux = BN_new();
		BN_mod_mul_reciprocal(aux, res, elems[indexes[i]], recp, ctx);
		res = BN_copy(res, aux);
		BN_free(aux);
	}

	BN_CTX_end(ctx);
	BN_CTX_free(ctx);
	BN_RECP_CTX_free(recp);

	return res;
}

BIGNUM ** arr_mod_subs(BIGNUM ** arr, BIGNUM * sub, const BIGNUM * m, long unsigned int size){
	BIGNUM ** subs = (BIGNUM **) malloc(sizeof(BIGNUM *)*size);
	long unsigned int i;

	BN_CTX * ctx = BN_CTX_new();
	BN_CTX_start(ctx);

	for(i = 0; i < size; i++){
		subs[i] = BN_new();
		BN_mod_sub(subs[i], sub, arr[i], m, ctx);
	}

	BN_CTX_end(ctx);
	BN_CTX_free(ctx);

	return subs;
}

BIGNUM * generate_big_prime(long unsigned int bits){
	BIGNUM * ret = BN_new();
	
	if(!BN_generate_prime_ex(ret, bits, 1, NULL, NULL, NULL)) {
		perror("BN_generate_prime_ex FAILED\n");
	}

	return ret;
}

BIGNUM ** generate_n_big_primes(long unsigned int n, long unsigned int bits){
	BIGNUM ** big_prms = (BIGNUM **) malloc(sizeof(BIGNUM *)*n);
	long unsigned int i;

	for(i = 0; i < n; i++){
		big_prms[i] = generate_big_prime(bits);
	}

	return big_prms;
}

BIGNUM * convert_int2bn(long unsigned int n){
	char n2str[32];
	sprintf(n2str, "%lu", n);

	BIGNUM * ret = generate_big_prime(bn_size);
	BN_dec2bn(&ret, n2str);

	return ret;
}

void BNs_free(BIGNUM ** array, long unsigned int size){
	long unsigned int i;

	for(i = 0; i < size; i++){
		BN_free(array[i]);
	}
	free(array);
}

void init_folder_files(int num_files){
	long unsigned int i;

	FILE * fp;

	char folder_name[50];
	char number[20];
	char temp[50];

	struct stat st = {0};

	if(stat("shares", &st) == -1) {
		mkdir("shares", 0700);
	}

	strcpy(folder_name, "shares");

	if(stat(folder_name, &st) == -1){
		mkdir(folder_name, 0700);
	}

	strcat(folder_name, "/share_");

	for(i = 0; i < num_files; i++){
		strcpy(temp, folder_name);
		sprintf(number, "%ld", i);
		strcat(temp, number);
		fp = fopen(temp, "w+");
		strcpy(temp, folder_name);
		fclose(fp);
	}
}

void init(){

}

void write_layer_bignum(BIGNUM ** shrs, long unsigned int k, const BIGNUM * m){
	long unsigned int i;
	char file_name[100];
	char file_din[100];
	char number[20];
	FILE * fp;
	char buffer[chunk_size*2 + 1];

	char bdbg[chunk_size + 1];

	strcpy(file_name, "shares/share_");

	for(i = 0; i < k; i++){
		bzero(buffer, sizeof(buffer));
		strcpy(file_din, file_name);

		sprintf(number, "%ld", i);
		strcat(file_din, number);

		fp = fopen(file_din, "a+");

		if (ferror(fp)) {
        	fprintf(stderr, "%s No such file or directory.\n", file_din);
    	}

		if (m != NULL) {
			buffer[num_chunk_size] = '\0';
			sprintf(buffer, "%lu", BN_num_bytes(m));
			fwrite(buffer, 1, num_chunk_size, fp);
        	
        	buffer[chunk_size*2] = '\0';
        	BN_bn2bin(m, buffer);
        	fwrite(buffer, 1, chunk_size*2, fp);
		} else {
			buffer[num_chunk_size] = '\0';
			sprintf(buffer, "%lu", BN_num_bytes(shrs[i]));
			fwrite(buffer, 1, num_chunk_size, fp);

    		buffer[chunk_size] = '\0';
        	BN_bn2bin(shrs[i], buffer);

			fwrite(buffer, 1, chunk_size, fp);
		}

		fclose(fp);
	}
}

void write_layer_num(long unsigned int size, long unsigned int k){
	long unsigned int i;
	char file_name[100];
	char file_din[100];
	char number[20];
	FILE * fp;
	char buffer[num_chunk_size + 1];

	strcpy(file_name, "shares/share_");

	for(i = 0; i < k; i++){
		strcpy(file_din, file_name);

		sprintf(number, "%ld", i);
		strcat(file_din, number);

		fp = fopen(file_din, "a+");

		if (ferror(fp)) {
        	fprintf(stderr, "%s No such file or directory.\n", file_din);
    	}

    	bzero(buffer, sizeof(buffer));
		buffer[num_chunk_size] = '\0';
		sprintf(buffer, "%lu", size);
		fwrite(buffer, 1, num_chunk_size, fp);

		fclose(fp);
	}
}

void secret_from_files(long unsigned int n, long unsigned int k){
	long unsigned int i = 2;
	int fin = 0;
	BIGNUM ** shrs;
	BIGNUM * secret;

	char * buffer = (char *) malloc(chunk_size*2 + 1);

	FILE * revealed = fopen("shares/secret", "a+");

	long unsigned int f_size = read_file_size();
	// leer M y Vs de fold
	BIGNUM ** m = read_layer(k, 0);
	BIGNUM ** vs = read_layer(k, 1);

	while(!fin){
		shrs = read_layer(5, i);
		if(shrs == NULL){
			fin = 1;
		} else {
			secret = compute_secret(shrs, vs, n, m[0]);
			// write secret to a file
			buffer[BN_num_bytes(secret)] = '\0';
			BN_bn2bin(secret, buffer);

			fwrite(buffer, 1, BN_num_bytes(secret), revealed);
			
			BNs_free(shrs, k);
			BN_free(secret);
			i++;
		}
	}

	printf("Secret decyphered correctly\n");

	fclose(revealed);
	BNs_free(vs, k);
	BNs_free(m, k);
	free(buffer);
}

BIGNUM ** read_layer(long unsigned int k, long unsigned int layer_offset){
	BIGNUM ** bns = (BIGNUM **) malloc(sizeof(BIGNUM *)*k);
	

	FILE * fp;
	char file_name[200];
	char folder_name[200];
	char number[20];
	char * buffer = (char *) malloc(chunk_size*2 + 1);
	long unsigned int i, size;
	size_t nread;
	char last_layer = 0;
	long unsigned int s_nex_ptr, s_cur_ptr, sz;

	strcpy(folder_name, "shares/share_");

	for(i = 0; (i < k) && !last_layer; i++){

		strcpy(file_name, folder_name);

		sprintf(number, "%ld", i);
		strcat(file_name, number);

		fp = fopen(file_name, "r");

		fseek(fp, 0L, SEEK_END);
		sz = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
		
		s_cur_ptr = layer_offset?(layer_offset-1)*(chunk_size+num_chunk_size)+(chunk_size*2+num_chunk_size*2) : 
				num_chunk_size;
		
		s_nex_ptr = s_cur_ptr + num_chunk_size+chunk_size;
		//fprintf(stdout, "%d - %d = %d\n", s_nex_ptr, sz, s_nex_ptr-sz);
		if(sz >= s_nex_ptr){
			// set fseek
			fseek(fp, s_cur_ptr, SEEK_SET);
			
			buffer[num_chunk_size] = '\0';
			fread(buffer, 1, num_chunk_size, fp);
			size = atol(buffer);
			bzero(buffer, sizeof(buffer));
			buffer[size] = '\0';
			fread(buffer, 1, size, fp);
			bns[i] = BN_new();
			BN_bin2bn(buffer, size, bns[i]);
		} else {
			last_layer = 1;
			free(bns);
			bns = NULL;
		}
		fclose(fp);
	}

	return bns;
}

long unsigned int read_file_size(){
	FILE * fp;

	char folder_name[200];
	char buffer[chunk_size*2 + 1];
	long unsigned int size;

	strcpy(folder_name, "shares/share_0");

	bzero(buffer, sizeof(buffer));
	
	fp = fopen(folder_name, "r");
	buffer[num_chunk_size] = '\0';
	fread(buffer, 1, num_chunk_size, fp);
	
	size = atol(buffer);

	fclose(fp);

	return size;
}

int start_thread(char * file, long unsigned int n, long unsigned int k){
	int i = 0;

	// Se crean k pequeños primos 
	BIGNUM ** little_vs = get_n_first_primes(k);
	// Luego n big primes
	BIGNUM ** big_primes = generate_n_big_primes(n, bn_size);
	// se crea el modulo para el sistema
	BIGNUM * m = generate_big_prime(bn_size + 16);
	// variable para las comparticiones
	BIGNUM ** shares;
	
	FILE * fp;
	// Abrimos el fichero y averiguamos su tamaño
	fp = fopen(file, "r");
	fseek(fp, 0L, SEEK_END);
	long unsigned int sz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	printf("File size %lu bytes\n", sz);

	// bufer de lectura y numero de bytes leidos
	char * buffer = (char *) malloc(chunk_size + 1);
	size_t nread, nread_cum = 0;

	// inicializamos ficheros y carpetas para su escritura posterior
	init_folder_files(k);

	// Primero escribimos el tamaño del fichero
	write_layer_num(sz, k);

	// Luego se escriben M (módulo) y Vs
	write_layer_bignum(NULL, k, m);
	write_layer_bignum(little_vs, k, NULL);

	// un bignum auxiliar 
	BIGNUM * temp_bn = BN_new();

	if (fp) {
		// leemos el fuchero al buffer
    	while ((nread = fread(buffer, 1, chunk_size, fp)) > 0){
        	nread_cum += nread;
        	// convertimos el buffer en el bignum
        	BN_bin2bn(buffer, nread, temp_bn);
        	buffer[BN_num_bytes(temp_bn)] = '\0';
        	
        	fprintf(stdout ,"Working %.1f%%\n", (float) nread_cum/sz*100);
        	// calculamos comparticiones para el pedazo leido
        	shares = compute_shares(big_primes, little_vs, k, n, temp_bn, m);
        	// escribimos las comparticiones en los ficheros creados
        	write_layer_bignum(shares, k, NULL);
        	// liberamos la memoria alocada para las comparticiones
        	BNs_free(shares, k);
        	bzero(buffer, sizeof(buffer));
    	}
    	
    	printf("Shares created succesfully\n");
	} else if (ferror(fp)){
		printf("Error creating shares\n");
	}

	// limpiamos los recursos
	fclose(fp);
	free(buffer);
	BN_free(temp_bn);
	BN_free(m);
	BNs_free(little_vs, k);
	BNs_free(big_primes, n);

	return 0;
}

void ex_test(char * fold){
	int k = 5;

	start_thread(fold, 3, k);

	secret_from_files(3, k);

}