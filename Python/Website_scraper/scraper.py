import requests
import json

url_template = "https://thanhnien.vn/api/get-data-tuyen-sinh.htm?keywords={keyword}&pageindex=1&size=10&type=2"
output_file = "results.txt"

with open(output_file, "w", encoding="utf-8") as f:
    for i in range(2500, 7357):
        keyword = str(i)
        url = url_template.format(keyword=keyword)
        try:
            response = requests.get(url)
            response.raise_for_status()
            data = response.json()
            json.dump(data, f, ensure_ascii=False, indent=2)

            print(f"Fetched {keyword}")
        except requests.RequestException as e:
            f.write(f"Keyword: {keyword} - Request failed: {e}\n\n")
            print(f"Failed {keyword}")
        except json.JSONDecodeError as e:
            f.write(f"Keyword: {keyword} - JSON decode failed: {e}\n\n")
            print(f"Decode failed {keyword}")
