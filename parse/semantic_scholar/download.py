import argparse
import requests
import os
from urllib.parse import urlparse
from tqdm import tqdm

API_KEY = ''

def get_citation_urls():
    headers = {
        'x-api-key': API_KEY,
    }
    response = requests.get('https://api.semanticscholar.org/datasets/v1/release/latest/dataset/citations', headers=headers).json()
    return response['files']
    

def get_paper_urls():
    headers = {
        'x-api-key': API_KEY,
    }
    response = requests.get('https://api.semanticscholar.org/datasets/v1/release/latest/dataset/papers', headers=headers).json()
    return response['files']


def download_one_file(url, file_path):
    response = requests.get(url)
    response.raise_for_status()
    with open(file_path, 'wb') as f:
        f.write(response.content)


def download_citation_files(path):
    urls = get_citation_urls()
    num_files = len(urls)
    print(f'Found {num_files} citation files to download.')
    for i in tqdm(range(num_files), desc='Downloading citation files'):
        url = urls[i]
        file_path = os.path.join(path, f'citation_file_{i}.gz')
        success = False
        for attempt in range(3):
            try:
                if not os.path.exists(file_path):
                    download_one_file(url, file_path)
                success = True
                break
            except Exception as e:
                urls = get_citation_urls()
        assert success, f'Failed to download {file_path} after 3 attempts.'


def download_paper_files(path):
    urls = get_paper_urls()
    num_files = len(urls)
    print(f'Found {num_files} paper files to download.')
    for i in tqdm(range(num_files), desc='Downloading paper files'):
        url = urls[i]
        file_path = os.path.join(path, f'paper_file_{i}.gz')
        success = False
        for attempt in range(3):
            try:
                if not os.path.exists(file_path):
                    download_one_file(url, file_path)
                success = True
                break
            except Exception as e:
                urls = get_paper_urls()
        assert success, f'Failed to download {file_path} after 3 attempts.'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--path',
        type=str,
        required=True,
        help='Path to download the files.'
    )
    args = parser.parse_args()

    if not os.path.exists(args.path):
        os.makedirs(args.path)

    download_citation_files(args.path)
    download_paper_files(args.path)


if __name__ == '__main__':
    main()
