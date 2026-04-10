#include <stdlib.h>

char	*ft_strchr(const char *s, int c) {
	char	find;
	size_t	i;

	i = 0;
	find = (char)c;
	while (s[i]) {
		if (s[i] == find)
			return ((char *)s + i);
		i++;
	}
	if (s[i] == find)
		return ((char *)s + i);
	return (0);
}

char	*ft_free(char *allocate)
{
	if (allocate)
		free(allocate);
	return (NULL);
}

char	*ft_join(char *data, char	*buffer)
{
	long	data_len;
	long	buf_len;
	long	i;
	long	j;
	char	*new_data;

	if (!data)
		data_len = 0;
	else
		data_len = ft_strchr(data, '\0') - data;
	buf_len = ft_strchr(buffer, '\0') - buffer;
	new_data = (char *)malloc((buf_len + data_len + 1) * sizeof(char));
	if (!new_data)
		return (NULL);
	i = -1;
	while (++i < data_len)
		new_data[i] = data[i];
	j = -1;
	while (++j < buf_len)
		new_data[i++] = buffer[j];
	new_data[i] = '\0';
	ft_free(data);
	return (new_data);
}