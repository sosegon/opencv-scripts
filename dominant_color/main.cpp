#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include <queue>
#include <string>
#include <iostream>
#include <dirent.h>
#include <fstream>

using namespace std;

string eyes_cascade_name = "../haarcascade_eye_tree_eyeglasses.xml";
cv::CascadeClassifier eyes_cascade;

typedef struct t_color_node {
    cv::Mat       mean;       // The mean of this node
    cv::Mat       cov;
    uchar         classid;    // The class ID

    t_color_node  *left;
    t_color_node  *right;
} t_color_node;

cv::Mat get_dominant_palette(std::vector<cv::Vec3b> colors) {
    const int tile_size = 64;
    cv::Mat ret = cv::Mat(tile_size, tile_size*colors.size(), CV_8UC3, cv::Scalar(0));

    for(int i=0;i<colors.size();i++) {
        cv::Rect rect(i*tile_size, 0, tile_size, tile_size);
        cv::rectangle(ret, rect, cv::Scalar(colors[i][0], colors[i][1], colors[i][2]), CV_FILLED);
    }

    return ret;
}

std::vector<t_color_node*> get_leaves(t_color_node *root) {
    std::vector<t_color_node*> ret;
    std::queue<t_color_node*> queue;
    queue.push(root);

    while(queue.size() > 0) {
        t_color_node *current = queue.front();
        queue.pop();

        if(current->left && current->right) {
            queue.push(current->left);
            queue.push(current->right);
            continue;
        }

        ret.push_back(current);
    }

    return ret;
}

std::vector<cv::Vec3b> get_dominant_colors(t_color_node *root) {
    std::vector<t_color_node*> leaves = get_leaves(root);
    std::vector<cv::Vec3b> ret;

    for(int i=0;i<leaves.size();i++) {
        cv::Mat mean = leaves[i]->mean;
        ret.push_back(cv::Vec3b(mean.at<double>(0)*255.0f,
                                mean.at<double>(1)*255.0f,
                                mean.at<double>(2)*255.0f));
    }

    return ret;
}

int get_next_classid(t_color_node *root) {
    int maxid = 0;
    std::queue<t_color_node*> queue;
    queue.push(root);

    while(queue.size() > 0) {
        t_color_node* current = queue.front();
        queue.pop();

        if(current->classid > maxid)
            maxid = current->classid;

        if(current->left != NULL)
            queue.push(current->left);

        if(current->right)
            queue.push(current->right);
    }

    return maxid + 1;
}

void get_class_mean_cov(cv::Mat img, cv::Mat classes, t_color_node *node) {
    const int width = img.cols;
    const int height = img.rows;
    const uchar classid = node->classid;

    cv::Mat mean = cv::Mat(3, 1, CV_64FC1, cv::Scalar(0));
    cv::Mat cov = cv::Mat(3, 3, CV_64FC1, cv::Scalar(0));

    // We start out with the average color
    double pixcount = 0;
    for(int y=0;y<height;y++) {
        cv::Vec3b* ptr = img.ptr<cv::Vec3b>(y);
        uchar* ptrClass = classes.ptr<uchar>(y);
        for(int x=0;x<width;x++) {
            if(ptrClass[x] != classid)
                continue;

            cv::Vec3b color = ptr[x];
            cv::Mat scaled = cv::Mat(3, 1, CV_64FC1, cv::Scalar(0));
            scaled.at<double>(0) = color[0]/255.0f;
            scaled.at<double>(1) = color[1]/255.0f;
            scaled.at<double>(2) = color[2]/255.0f;

            mean += scaled;
            cov = cov + (scaled * scaled.t());

            pixcount++;
        }
    }

    cov = cov - (mean * mean.t()) / pixcount;
    mean = mean / pixcount;

    // The node mean and covariance
    node->mean = mean.clone();
    node->cov = cov.clone();

    return;
}

void partition_class(cv::Mat img, cv::Mat classes, uchar nextid, t_color_node *node) {
    const int width = img.cols;
    const int height = img.rows;
    const int classid = node->classid;

    const uchar newidleft = nextid;
    const uchar newidright = nextid+1;

    cv::Mat mean = node->mean;
    cv::Mat cov = node->cov;
    cv::Mat eigenvalues, eigenvectors;
    cv::eigen(cov, eigenvalues, eigenvectors);

    cv::Mat eig = eigenvectors.row(0);
    cv::Mat comparison_value = eig * mean;

    node->left = new t_color_node();
    node->right = new t_color_node();

    node->left->classid = newidleft;
    node->right->classid = newidright;

    // We start out with the average color
    for(int y=0;y<height;y++) {
        cv::Vec3b* ptr = img.ptr<cv::Vec3b>(y);
        uchar* ptrClass = classes.ptr<uchar>(y);
        for(int x=0;x<width;x++) {
            if(ptrClass[x] != classid)
                continue;

            cv::Vec3b color = ptr[x];
            cv::Mat scaled = cv::Mat(3, 1,
                                  CV_64FC1,
                                  cv::Scalar(0));

            scaled.at<double>(0) = color[0]/255.0f;
            scaled.at<double>(1) = color[1]/255.0f;
            scaled.at<double>(2) = color[2]/255.0f;

            cv::Mat this_value = eig * scaled;

            if(this_value.at<double>(0, 0) <= comparison_value.at<double>(0, 0)) {
                ptrClass[x] = newidleft;
            } else {
                ptrClass[x] = newidright;
            }
        }
    }
    return;
}

cv::Mat get_quantized_image(cv::Mat classes, t_color_node *root) {
    std::vector<t_color_node*> leaves = get_leaves(root);

    const int height = classes.rows;
    const int width = classes.cols;
    cv::Mat ret(height, width, CV_8UC3, cv::Scalar(0));

    for(int y=0;y<height;y++) {
        uchar *ptrClass = classes.ptr<uchar>(y);
        cv::Vec3b *ptr = ret.ptr<cv::Vec3b>(y);
        for(int x=0;x<width;x++) {
            uchar pixel_class = ptrClass[x];
            for(int i=0;i<leaves.size();i++) {
                if(leaves[i]->classid == pixel_class) {
                    ptr[x] = cv::Vec3b(leaves[i]->mean.at<double>(0)*255,
                                       leaves[i]->mean.at<double>(1)*255,
                                       leaves[i]->mean.at<double>(2)*255);
                }
            }
        }
    }

    return ret;
}

cv::Mat get_viewable_image(cv::Mat classes) {
    const int height = classes.rows;
    const int width = classes.cols;

    const int max_color_count = 12;
    cv::Vec3b *palette = new cv::Vec3b[max_color_count];
    palette[0]  = cv::Vec3b(  0,   0,   0);
    palette[1]  = cv::Vec3b(255,   0,   0);
    palette[2]  = cv::Vec3b(  0, 255,   0);
    palette[3]  = cv::Vec3b(  0,   0, 255);
    palette[4]  = cv::Vec3b(255, 255,   0);
    palette[5]  = cv::Vec3b(  0, 255, 255);
    palette[6]  = cv::Vec3b(255,   0, 255);
    palette[7]  = cv::Vec3b(128, 128, 128);
    palette[8]  = cv::Vec3b(128, 255, 128);
    palette[9]  = cv::Vec3b( 32,  32,  32);
    palette[10] = cv::Vec3b(255, 128, 128);
    palette[11] = cv::Vec3b(128, 128, 255);

    cv::Mat ret = cv::Mat(height, width, CV_8UC3, cv::Scalar(0, 0, 0));
    for(int y=0;y<height;y++) {
        cv::Vec3b *ptr = ret.ptr<cv::Vec3b>(y);
        uchar *ptrClass = classes.ptr<uchar>(y);
        for(int x=0;x<width;x++) {
            int color = ptrClass[x];
            if(color >= max_color_count) {
                printf("You should increase the number of predefined colors!\n");
                continue;
            }
            ptr[x] = palette[color];
        }
    }

    return ret;
}

t_color_node* get_max_eigenvalue_node(t_color_node *current) {
    double max_eigen = -1;
    cv::Mat eigenvalues, eigenvectors;

    std::queue<t_color_node*> queue;
    queue.push(current);

    t_color_node *ret = current;
    if(!current->left && !current->right)
        return current;

    while(queue.size() > 0) {
        t_color_node *node = queue.front();
        queue.pop();

        if(node->left && node->right) {
            queue.push(node->left);
            queue.push(node->right);
            continue;
        }

        cv::eigen(node->cov, eigenvalues, eigenvectors);
        double val = eigenvalues.at<double>(0);
        if(val > max_eigen) {
            max_eigen = val;
            ret = node;
        }
    }

    return ret;
}

std::vector<cv::Vec3b> find_dominant_colors(cv::Mat img, int count) {
    const int width = img.cols;
    const int height = img.rows;

    cv::Mat classes = cv::Mat(height, width, CV_8UC1, cv::Scalar(1));
    t_color_node *root = new t_color_node();

    root->classid = 1;
    root->left = NULL;
    root->right = NULL;

    t_color_node *next = root;
    get_class_mean_cov(img, classes, root);
    for(int i=0;i<count-1;i++) {
        next = get_max_eigenvalue_node(root);
        partition_class(img, classes, get_next_classid(root), next);
        get_class_mean_cov(img, classes, next->left);
        get_class_mean_cov(img, classes, next->right);
    }

    std::vector<cv::Vec3b> colors = get_dominant_colors(root);

//    cv::Mat quantized = get_quantized_image(classes, root);
//    cv::Mat viewable = get_viewable_image(classes);
//    cv::Mat dom = get_dominant_palette(colors);
//
//    cv::imwrite("./classification.png", viewable);
//    cv::imwrite("./quantized.png", quantized);
//    cv::imwrite("./palette.png", dom);

    return colors;
}

cv::Mat get_eye(cv::Mat matFace) {
	cv::Mat matFaceGray;
	vector<cv::Rect> eyes;

	cv::cvtColor(matFace, matFaceGray, CV_BGR2GRAY);
	cv::equalizeHist(matFaceGray, matFaceGray);

	eyes_cascade.detectMultiScale(matFaceGray, eyes, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, cv::Size(15, 15));

	cv::Mat matEye;

	if(eyes.size() > 0) {
		matEye = matFace(eyes[0]);
	}

	return matEye;
}

char* dominant_colors_as_string(std::vector<cv::Vec3b> colors) {
    char comma[] = ",";
    char *result;
    char component_buffer [33]; // 4 bytes x 8 bits + 1

    result = (char*)malloc(colors.size() * 12 + 1); // 12 characteres per color
    char init[] = "";
    strcpy(result, init);

    for(int i = 0; i < colors.size(); i++) {
        snprintf(component_buffer, sizeof(component_buffer), "%d", colors[i][0]);
        strcat(result, component_buffer);
        strcat(result, comma);
        snprintf(component_buffer, sizeof(component_buffer), "%d", colors[i][1]);
        strcat(result, component_buffer);
        strcat(result, comma);
        snprintf(component_buffer, sizeof(component_buffer), "%d", colors[i][2]);
        strcat(result, component_buffer);
        if(i < colors.size() - 1) {
            strcat(result, comma);
        }
    }

    return result;
}

std::vector<char*> extract_dominant_colors(char* filename, int count_face = 6, int count_eye = 3) {
    cv::Mat mat_face = cv::imread(filename);

    if(mat_face.empty()) {
        throw 1;
    }

    cv::Mat mat_eye = get_eye(mat_face);

    if(mat_eye.empty()) {
        throw 1;
    }

    std::vector<cv::Vec3b> eye_colors = find_dominant_colors(mat_eye, count_eye);
    std::vector<cv::Vec3b> face_colors = find_dominant_colors(mat_face, count_face);

    char *result_eye;
    result_eye = dominant_colors_as_string(eye_colors);
    char *result_face;
    result_face = dominant_colors_as_string(face_colors);

    std::vector<char*> result;
    result.push_back(result_face);
    result.push_back(result_eye);

    return result;
}

void save_to_file(char* folder_name, char* file_name, char* image_file, char* values) {
    ofstream file;

    char* full_name;
    full_name = (char*)malloc(strlen(folder_name) + strlen(file_name) + 2);

    strcpy(full_name, folder_name);
    strcat(full_name, "/");
    strcat(full_name, file_name);

    file.open(full_name, ios::app);

    if(file.is_open()) {
        file << image_file << "," << values << "\n";
    }

    file.close();
}

int main(int argc, char* argv[]) {
    int count_face, count_eye;

    if(argc < 2) {
        printf("Usage: %s <folder> <count_face> <count_eye>\n", argv[0]);
        return 0;
    } else if(argc < 3) {
        count_face = 6;
        count_eye = 3;
    } else if(argc < 4) {
        count_eye = 3;
        count_face = atoi(argv[2]);
    } else {
        count_face = atoi(argv[2]);
        count_eye = atoi(argv[3]);
    }

    if(!eyes_cascade.load(eyes_cascade_name)) {
        printf("Error loading cascade");
        return 4;
    }

    DIR *dir;
    struct dirent *ent;

    char* full_name;

    if((dir = opendir(argv[1])) != NULL) {
    	while((ent = readdir(dir)) != NULL) {
    	    full_name = (char*)malloc(strlen(argv[1]) + strlen(ent->d_name) + 2);
            strcpy(full_name, argv[1]);
            strcat(full_name, "/");
            strcat(full_name, ent->d_name);

    		try {
                std::vector<char*> result = extract_dominant_colors(full_name);
                save_to_file(argv[1], "face_colors.txt", ent->d_name, result[0]);
                save_to_file(argv[1], "eye_colors.txt", ent->d_name, result[1]);

                free(result[0]);
                free(result[1]);

            } catch(int a) {
    		    printf("Not processed: %s\n", full_name);
    		}

    	}
    	closedir(dir);
    } else {
    	perror("");
    	return EXIT_FAILURE;
    }

    return 0;
}
